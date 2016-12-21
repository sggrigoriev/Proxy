//
// Created by gsg on 07/12/16.
//
#include <pthread.h>
#include <string.h>
#include <malloc.h>
#include <lib_http.h>
#include <pf_traffic_proc.h>

#include "wc_defaults.h"
#include "pu_logger.h"
#include "wt_queues.h"
#include "wt_http_utl.h"

#include "wt_server_write.h"

#define PT_THREAD_NAME "SERVER_WRITE"

////////////////////////////
static pthread_t id;
static pthread_attr_t attr;
static volatile int stop;
static pu_queue_msg_t msg[WC_MAX_MSG_LEN];

static pu_queue_t* to_server;       //to write

static void* write_proc(void* params);

int wt_start_server_write() {
    if(pthread_attr_init(&attr)) return 0;
    if(pthread_create(&id, &attr, &write_proc, NULL)) return 0;
    return 1;
}

void wt_stop_server_write() {
    void *ret;

    wt_set_stop_server_write();
    pthread_join(id, &ret);
    pthread_attr_destroy(&attr);
}

void wt_set_stop_server_write() {
    stop = 1;
}

static void* write_proc(void* params){
    pu_queue_event_t events;

    stop = 0;
    to_server = wt_get_gueue(WT_AlertsToCloud);
    events = pu_add_queue_event(pu_create_event_set(), WT_AlertsToCloud);
    while(!stop) {
        pu_queue_event_t ev;
        switch (ev = pu_wait_for_queues(events, WUD_DEFAULT_SERVER_WRITE_TO_SEC)) {
            case WT_AlertsToCloud: {
                size_t len = sizeof(msg);
                while (pu_queue_pop(to_server, msg, &len)) {
                    pu_log(LL_DEBUG, "%s: Got from from main by server_write_thread: %s", PT_THREAD_NAME, msg);
//Sending with retries loop
                    int out = 0;
                    int retries = LIBB_HTTP_MAX_POST_RETRIES;
    //Adding head to the message
                    char devid[LIB_HTTP_DEVICE_ID_SIZE];
                    wt_get_device_id(devid, sizeof(devid));
                    pf_add_proxy_head(msg, sizeof(msg), devid, 11039);

                    while(!stop && !out) {
                        char resp[LIB_HTTP_MAX_MSG_SIZE];

                        switch (wt_http_write(msg, sizeof(msg), resp, sizeof(resp))) {
                            case LIB_HTTP_POST_ERROR:
                                pu_log(LL_ERROR, "%s: Error sending", PT_THREAD_NAME);
                                out = 1;
                                break;
                            case LIB_HTTP_POST_RETRY:
                                pu_log(LL_WARNING, "%s: Connectivity problems, retry", PT_THREAD_NAME);
                                if(retries-- == 0) {
                                    char conn_str[LIB_HTTP_MAX_URL_SIZE];
                                    pu_log(LL_ERROR,  "%s: can't connect to %s. Message is not sent. %s",
                                           PT_THREAD_NAME, wt_http_get_cloud_conn_string(conn_str, sizeof(conn_str)), msg);
                                    out = 1;
                                }
                                else {
                                    sleep(1);
                                }
                                break;
                            case LIB_HTTP_POST_OK:
                                pu_log(LL_INFO, "%s: Sent to cloud: %s", PT_THREAD_NAME, msg);
                                if (strlen(resp) > 0) {
                                    pu_log(LL_INFO, "%s: Answer from cloud: %s", PT_THREAD_NAME, resp);
                                    out = 1;
                                }
                                 break;
                            default:
                                break;
                        }
                    }
                }
                break;
            }
            case WT_Timeout:
                pu_log(LL_WARNING, "%s: timeout on waiting to server queue", PT_THREAD_NAME);
                break;
            case WT_STOP:
                stop = 1;
                pu_log(LL_INFO, "%s received STOP event. Terminated", PT_THREAD_NAME);
                break;
            default:
                pu_log(LL_ERROR, "%s: Undefined event %d on wait (to server)!", PT_THREAD_NAME, ev);
                break;
        }

    }
    pu_log(LL_INFO, "%s is finished", PT_THREAD_NAME);
    pthread_exit(NULL);
}