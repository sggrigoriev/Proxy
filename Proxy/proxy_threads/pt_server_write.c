//
// Created by gsg on 06/12/16.
//
#include <pthread.h>

#include "pc_defaults.h"
#include "pu_logger.h"
#include "pt_http_utl.h"
#include "pt_queues.h"
#include "pt_server_write.h"

#define PT_THREAD_NAME "SERVER_WRITE"

////////////////////////////
static pthread_t id;
static pthread_attr_t attr;
static volatile int stop;
static pu_queue_msg_t msg[PROXY_MAX_MSG_LEN];

static pu_queue_t* to_main;       //to write answers from cloud
static pu_queue_t* from_main;     //to read and pass

static void* write_proc(void* params);

int start_server_write() {
    if(pthread_attr_init(&attr)) return 0;
    if(pthread_create(&id, &attr, &write_proc, NULL)) return 0;
    return 1;
}

void stop_server_write() {
    void *ret;

    set_stop_server_write();
    pthread_join(id, &ret);
    pthread_attr_destroy(&attr);
}

void set_stop_server_write() {
    stop = 1;
}

static void* write_proc(void* params) {
    pu_queue_event_t events;

    stop = 0;
    from_main = pt_get_gueue(PS_ToServerQueue);
    to_main = pt_get_gueue(PS_FromServerQueue);

    if(!pt_http_write_init()) {
        pu_log(LL_ERROR, "%s: Cloud info write initiation failed. Stop.", PT_THREAD_NAME);
        pthread_exit(NULL);
    }

    events = pu_add_queue_event(pu_create_event_set(), PS_ToServerQueue);

//Main write loop
    while(!stop) {
        pu_queue_event_t ev;
        switch (ev = pu_wait_for_queues(events, DEFAULT_SERVER_WRITE_THREAD_TO_SEC)) {
            case PS_ToServerQueue: {
                size_t len = sizeof(msg);
                while (pu_queue_pop(from_main, msg, &len)) {
                    pu_log(LL_DEBUG, "%s: Got from from main by server_write_thread: %s", PT_THREAD_NAME, msg);

//Sending with retries loop
                    int out = 0;
                    while(!stop && !out) {
                        char *resp = NULL;
                        size_t resp_len = 0;

                        switch (pt_http_write(msg, len, &resp, &resp_len)) {
                            case PT_POST_ERROR:
                                pu_log(LL_ERROR, "%s: Error sending: %s", PT_THREAD_NAME, resp);
                                out = 1;
                                break;
                            case PT_POST_RETRY:
                                pu_log(LL_WARNING, "%s: Connectivity problems, retry. %s", PT_THREAD_NAME, resp);
                                sleep(1);
                                break;
                            case PT_POST_OK:
                                pu_log(LL_INFO, "%s: Sent to cloud: %s", PT_THREAD_NAME, msg);
                                if (resp_len > 0) {
                                    pu_log(LL_INFO, "%s: Answer from cloud: %s", PT_THREAD_NAME, resp);
                                    pu_queue_push(to_main, resp, resp_len);
                                    out = 1;
                                }
                                len = sizeof(msg);
                                break;
                            default:
                                break;
                        }
                     }
                }
                break;
            }
            case PS_Timeout:
                pu_log(LL_WARNING, "%s: timeout on waiting to server queue", PT_THREAD_NAME);
                break;
            case PS_STOP:
                stop = 1;
                pu_log(LL_INFO, "%s received STOP event. Terminated", PT_THREAD_NAME);
                break;
            default:
                pu_log(LL_ERROR, "%s: Undefined event %d on wait (to server)!", PT_THREAD_NAME, ev);
                break;
        }
    }
    pt_http_write_destroy();
    pthread_exit(NULL);
}