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
#include <pf_traffic_proc.h>

#include "pt_queues.h"
#include "pu_logger.h"

#include "pc_defaults.h"
#include "pc_settings.h"
#include "ph_manager.h"
#include "pf_proxy_commands.h"

#include "pt_server_read.h"

#define PT_THREAD_NAME "CLOUD_READ"

/* Answers from cloud dumm emulator */
#ifdef PROXY_AUTH_GET_EMUL
static const char* emu = "{\"version\":\"2\",\"status\":\"ACK\",\"commands\":[{\"commandId\":463,\"type\":0,\"deviceId\":\"aioxGW-0000080027F78CD0\",\"parameters\":[{\"name\":\"permitJoining\",\"value\":\"1\"}]}]}";
#endif

/************************************************************************************
 * Local data
 */
static pthread_t id;
static pthread_attr_t attr;

static volatile int stop;       /* Thread stop flag */

static pu_queue_t* to_main;     /* Transport to main proxy thread */

/*****************************************************************************************
 * Local functions
 */
/*
 * Get the message from the cloud
 *  buf     - buffer for message received
 *  size    - buffer size
 */
static void read_from_cloud(char* buf, size_t size) {
    int out = 0;
    while(!out && !stop) {
        switch(ph_read(buf, size)) {
            case -1:        /*error*/
                pu_log(LL_ERROR, "%s: Error reading. Reconnect", PT_THREAD_NAME);
                pf_reconnect(to_main);    /* loop again the succ inside */
                out = 0;
                break;
            case 0:     /* timeout - read again */
#ifdef PROXY_AUTH_GET_EMUL
                strcpy(buf, emu);
                    out = 1;
#endif
                sleep(LIB_HTTP_DEFAULT_CONN_REESTABLISHMENT_DELAY_SEC);
                break;
            case 1:     /* got smth */
                out = 1;
                break;
            default:
                pu_log(LL_ERROR, "%s: unexpected rc from pt_http_read");
                break;
        }
    }
}
/*
 * Thread function
*/
static void* read_proc(void* params) {

    to_main = pt_get_gueue(PS_FromServerQueue);

    stop = 0;

    char buf[LIB_HTTP_MAX_MSG_SIZE];

/* Main read loop */
    while(!stop) {
        read_from_cloud(buf, sizeof(buf));
        pu_log(LL_DEBUG, "%s: received from cloud: %s", PT_THREAD_NAME, buf);
        pu_queue_push(to_main, buf, strlen(buf)+1); /* Forward the message ot the proxy_main */
/* Delay for some milliseconds to get time to Agent for response */
        {
            struct timespec t = {0, REGET_DELAY}, rem;
            nanosleep(&t, &rem);
        }
    }
    pu_log(LL_INFO, "%s: STOP. Terminated", PT_THREAD_NAME);
    pthread_exit(NULL);
}

/************************************************************************************************
 * Public functions impementation
 */
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
