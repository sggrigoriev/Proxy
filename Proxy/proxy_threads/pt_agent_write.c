/*
 *  Copyright 2017 People Power Company
 *
 *  This code was developed with funding from People Power Company
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */
/*
    Created by gsg on 06/12/16.
*/
#include <pthread.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>

#include "lib_tcp.h"
#include "pu_logger.h"
#include "pt_queues.h"

#include "pc_defaults.h"
#include "pt_main_agent.h"

#include "pt_agent_write.h"

#define PT_THREAD_NAME "AGENT_WRITE"

/*********************************************************************
 * Local data
 */
static pthread_t id;
static pthread_attr_t attr;

static volatile int stop;   /* one stop for write and read agent threads */

static char out_buf[PROXY_MAX_MSG_LEN]; /* buffer for write into the socket */

static int write_socket;                /* writable socket */
static pu_queue_t* from_main;           /* queue - the source of info to be written into the socket */

/* Thread function: get info from main thread, write it into the socket */
static void* agent_write(void* params) {
    from_main = pt_get_gueue(PS_ToAgentQueue);

/* Queue events init */
    pu_queue_event_t events;
    events = pu_add_queue_event(pu_create_event_set(), PS_ToAgentQueue);

/* Main loop */
    while(!is_childs_stop()) {
        pu_queue_event_t ev;

        switch(ev=pu_wait_for_queues(events, DEFAULT_S_TO)) {
            case PS_ToAgentQueue: {
                size_t len = sizeof(out_buf);
                while (pu_queue_pop(from_main, out_buf, &len)) {
                    /* Prepare write operation */
                    ssize_t ret;
                    while(ret = lib_tcp_write(write_socket, out_buf, len, 1), !ret&&!is_childs_stop());  /* run until the timeout */
                    if(is_childs_stop()) break; /* goto reconnect */
                    if(ret < 0) { /* op start failed */
                        pu_log(LL_ERROR, "%s. Write op failed %d %s. Reconnect", PT_THREAD_NAME, errno, strerror(errno));
/* Put back non-sent message */
                        pu_queue_push(from_main, out_buf, len);
                        set_stop_agent_children();
                        break;
                    }
                    pu_log(LL_DEBUG, "%s: written: %s", PT_THREAD_NAME, out_buf);
                    len = sizeof(out_buf);
                }
                break;
            }
            case PS_Timeout:
                break;
            case PS_STOP:
                set_stop_agent_children();
                pu_log(LL_INFO, "%s: received STOP event. Terminated", PT_THREAD_NAME);
                break;
            default:
                pu_log(LL_ERROR, "%s: Undefined event %d on wait!", PT_THREAD_NAME, ev);
                break;
        }
    }
    pu_log(LL_INFO, "%s is finished", PT_THREAD_NAME);
    lib_tcp_client_close(write_socket);
    pthread_exit(NULL);
}

int start_agent_write(int socket) {
    write_socket = socket;
    if(pthread_attr_init(&attr)) return 0;
    if(pthread_create(&id, &attr, &agent_write, NULL)) return 0;
    return 1;
}

void stop_agent_write() {
    void *ret;
    pthread_join(id, &ret);
    pthread_attr_destroy(&attr);

    set_stop_agent_children();
}



