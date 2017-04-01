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
    Created by gsg on 07/12/16.
*/
#include <string.h>
#include <pthread.h>

#include "pu_logger.h"
#include "lib_http.h"
#include "pf_traffic_proc.h"

#include "wc_defaults.h"
#include "wc_settings.h"
#include "wt_queues.h"
#include "wh_manager.h"

#include "wt_server_write.h"

#define PT_THREAD_NAME "SERVER_WRITE"

/*******************************************
 * Local data definition
 */
static pthread_t id;
static pthread_attr_t attr;

static volatile int stop;                   /* Thread stop flag */
static pu_queue_msg_t msg[WC_MAX_MSG_LEN];  /* Bufer for sending messages */

static pu_queue_t* to_cloud;       /* Thread communication channel with big WUD */

/*******************************************
 * Thread function
 */
static void* write_proc(void* params);

/*******************************************
 * Public functinos implementation
 */

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

/*******************************************
 * Local functinos implementation
 */


 static void* write_proc(void* params){
    pu_queue_event_t events;

    stop = 0;
    to_cloud = wt_get_gueue(WT_to_Cloud);
    events = pu_add_queue_event(pu_create_event_set(), WT_to_Cloud);
    while(!stop) {
        pu_queue_event_t ev;
        switch (ev = pu_wait_for_queues(events, 1)) {
            case WT_to_Cloud: {
                size_t len = sizeof(msg);
                while (pu_queue_pop(to_cloud, msg, &len)) {
                    pu_log(LL_DEBUG, "%s: Got from from main by server_write_thread: %s", PT_THREAD_NAME, msg);
/* Sending with reconnects loop */
                    int out = 0;
                    /* Add head to the message */
                    char devid[LIB_HTTP_DEVICE_ID_SIZE];
                    wc_getDeviceID(devid, sizeof(devid));
                    pf_add_proxy_head(msg, sizeof(msg), devid, 11039);

                    while (!stop && !out) {
                        char resp[LIB_HTTP_MAX_MSG_SIZE];

                        if (!wh_write(msg, resp, sizeof(resp))) {
                            pu_log(LL_ERROR, "%s: Error sending. Reconnect", PT_THREAD_NAME);
                            char conn_str[LIB_HTTP_MAX_URL_SIZE];
                            wc_getURL(conn_str, sizeof(conn_str));
                            wh_reconnect(conn_str, devid);
                            out = 0;
                        } else {
                            pu_log(LL_INFO, "%s: Sent to cloud: %s", PT_THREAD_NAME, msg);
                            if (strlen(resp) > 0) {
                                pu_log(LL_INFO, "%s: Answer from cloud: %s", PT_THREAD_NAME, resp);
                                out = 1;
                            }
                        }
                    } /* while (!stop && !out) */
                } /* while (pu_queue_pop(to_cloud, msg, &len)) */
                break;
            }
            case WT_Timeout:
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