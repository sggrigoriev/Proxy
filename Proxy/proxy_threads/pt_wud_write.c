//
// Created by gsg on 12/12/16.
//
#ifndef PROXY_SEPARATE_RUN

#include <pthread.h>
#include <string.h>
#include <errno.h>

#include "pc_settings.h"
#include "lib_tcp.h"
#include "pc_defaults.h"
#include "pt_queues.h"
#include "pt_wud_write.h"


#define PT_THREAD_NAME "WUD_WRITE"

////////////////////////////
static pthread_t id;
static pthread_attr_t attr;
static volatile int stop;   //one stop for write and read agent threads

static char out_buf[PROXY_MAX_MSG_LEN*2];

static pu_queue_t* to_wud;

static void* wud_write(void* params);

int start_wud_write() {
    if(pthread_attr_init(&attr)) return 0;
    if(pthread_create(&id, &attr, &wud_write, NULL)) return 0;
    return 1;
}

void stop_wud_write() {
    void *ret;
    set_stop_wud_write();
    
    pthread_join(id, &ret);
    pthread_attr_destroy(&attr);
}

void set_stop_wud_write() {
    stop = 1;
}
static void* wud_write(void* params) {
    to_wud = pt_get_gueue(PS_ToWUDQueue);

//Queue events init
    pu_queue_event_t events;
    events = pu_add_queue_event(pu_create_event_set(), PS_ToWUDQueue);

    while(!stop) {
        int write_socket = -1;

        while(write_socket = lib_tcp_get_client_socket(pc_getWUDPort(), 1), write_socket <= 0) {
            pu_log(LL_ERROR, "%s: connection error %d %s", PT_THREAD_NAME, errno, strerror(errno));
            sleep(1);
            continue;
        }
        pu_log(LL_DEBUG, "%s: Connected. Port = %d, socket = %d", PT_THREAD_NAME, pc_getWUDPort(), write_socket);

        int reconnect = 0;
        while (!stop) {
            pu_queue_event_t ev;
            switch (ev = pu_wait_for_queues(events, DEFAULT_WUD_WRITE_THREAD_TO_SEC)) {
                case PS_ToWUDQueue: {
                    size_t len = sizeof(out_buf);
                    while (pu_queue_pop(to_wud, out_buf, &len)) {
                        ssize_t ret;
                        while(ret = lib_tcp_write(write_socket, out_buf, len, 1), !ret&&!stop);  //run until the timeout
                        if(stop) break; // goto reconnect
                        if(ret < 0) { //op start failed
                            pu_log(LL_ERROR, "%s. Write op finish failed %d %s. Reconnect", PT_THREAD_NAME, errno, strerror(errno));
                            reconnect = 1;
                            break;
                        }
                        pu_log(LL_DEBUG, "%s: sent to WUD %s", PT_THREAD_NAME, out_buf);
                    }
                    break;
                }
                case PS_Timeout:    //currently TO=0 but...
//                          pu_log(LL_WARNING, "%s: timeout on waiting from server queue", PT_THREAD_NAME);
                    break;
                case PS_STOP:
                    set_stop_wud_write();
                    break;
                default:
                    pu_log(LL_ERROR, "%s: Undefined event %d on wait!", PT_THREAD_NAME, ev);
                    break;
            }
            if (reconnect) {
                lib_tcp_client_close(write_socket);
                pu_log(LL_WARNING, "%s: reconnect");
                break;  //inner while(!stop)
            }
        }
        lib_tcp_client_close(write_socket);
    }
    pu_log(LL_INFO, "%s is finished", PT_THREAD_NAME);
    pthread_exit(NULL);
}
#endif


