//
// Created by gsg on 06/12/16.
//
#include <pthread.h>
#include <string.h>

#include "pc_defaults.h"
#include "pu_logger.h"
#include "pt_http_utl.h"
#include "pt_queues.h"
#include "pt_server_read.h"

#define PT_THREAD_NAME "SERVER_READ"

////////////////////////////
static pthread_t id;
static pthread_attr_t attr;
static volatile int stop;

static pu_queue_t* to_main;

static void* read_proc(void* params);

int start_server_read() {
    if(pthread_attr_init(&attr)) return 0;
    if(pthread_create(&id, &attr, &read_proc, NULL)) return 0;
    return 1;
}

void stop_server_read() {
    void *ret;

    set_stop_server_read();
    pthread_join(id, &ret);
    pthread_attr_destroy(&attr);
}

void set_stop_server_read() {
    stop = 1;
}

static void* read_proc(void* params) {

    to_main = pt_get_gueue(PS_FromServerQueue);
    stop = 0;

    char buf[PROXY_MAX_MSG_LEN];

    if (!pt_http_read_init()) {
        pu_log(LL_ERROR, "%s: Cloud info read initiation failed. Stop.", PT_THREAD_NAME);
        pthread_exit(NULL);
    }

//Main read loop
    while(!stop) {
        int out = 0;
        while(!out && !stop) {
            switch(pt_http_read(buf, sizeof(buf))) {
                case -1:        //error
                    sleep(1);   //just not to have cycling
                    break;
                case 0:     //timeout - read again
                    break;
                case 1:     //got smth
                    out = 1;
                    break;
                default:
                    pu_log(LL_ERROR, "%s: unexpected rc from pt_http_read");
                    break;
            }
        }
        if(!stop) {
            pu_queue_push(to_main, buf, strlen(buf)+1);  //NB! Possibly needs to split info to 0-terminated strings!
            pu_log(LL_INFO, "Received from cloud: %s", buf);
        }
        else {
            pu_log(LL_INFO, "&s STOP. Terminated", PT_THREAD_NAME);
        }
    }
    pt_http_read_destroy();
    pthread_exit(NULL);
}

