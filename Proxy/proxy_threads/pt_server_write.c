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
    reated by gsg on 06/12/16.
*/
#include <pthread.h>
#include <string.h>
#include <proxy_functions/pf_proxy_commands.h>

#include "pu_logger.h"
#include "pt_queues.h"
#include "pc_defaults.h"
#include "pc_settings.h"
#include "pf_traffic_proc.h"
#include "ph_manager.h"
#include "pt_server_write.h"

#define PT_THREAD_NAME "CLOUD_WRITE"

/**********************************************************************
 * Local data
 */
static pthread_t id;
static pthread_attr_t attr;

static volatile int stop;                       /* Thread stip flag */
static pu_queue_msg_t msg[PROXY_MAX_MSG_LEN];   /* Buffer for sending message */

static pu_queue_t* to_cloud;        /* to read and forward to the cloud*/
static pu_queue_t* to_agent;        /* to send cloud answers to the agent_write */
static pu_queue_t* to_main;         /* send on/off-line notifications */

/*******************************************************************************
 * Local functions
*/
/* Thread function */
static void* write_proc(void* params);

/*
 * Public functions implementation
 */

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

/********************************************************************************
 * Local functions implementation
 */
static void* write_proc(void* params) {
    pu_queue_event_t events;

    stop = 0;
    to_cloud = pt_get_gueue(PS_ToServerQueue);
    to_agent = pt_get_gueue(PS_ToAgentQueue);
    to_main = pt_get_gueue(PS_FromServerQueue);

    events = pu_add_queue_event(pu_create_event_set(), PS_ToServerQueue);

    char devid[LIB_HTTP_DEVICE_ID_SIZE];
    char fwver[DEFAULT_FW_VERSION_SIZE];
    pc_getProxyDeviceID(devid, sizeof(devid));
    pc_getFWVersion(fwver, sizeof(fwver));

/* Main write loop */
    while(!stop) {
        pu_queue_event_t ev;
        switch (ev = pu_wait_for_queues(events, DEFAULT_SERVER_WRITE_THREAD_TO_SEC)) {
            case PS_ToServerQueue: {
                size_t len = sizeof(msg);
                while (pu_queue_pop(to_cloud, msg, &len)) {
 /* Sending with retries loop */
                    int out = 0;
    /* Adding the head to message */
                    pf_add_proxy_head(msg, sizeof(msg), devid);

                    while(!stop && !out) {
                        char resp[PROXY_MAX_MSG_LEN];
                        if(!ph_write(msg, resp, sizeof(resp))) {    /* no connection: reconnect forever */
                            pu_log(LL_ERROR, "%s: Error sending. Reconnect", PT_THREAD_NAME);
                            pf_reconnect(to_main);    /* loop until the succ inside */
                            out = 0;
                        }
                        else {  /* data has been written */
                            pu_log(LL_INFO, "%s: Sent to cloud: %s", PT_THREAD_NAME, msg);
                            if (strlen(resp) > 0) {
                                pu_log(LL_INFO, "%s: Answer from cloud forwarded to Agent: %s", PT_THREAD_NAME, resp);
                                pu_queue_push(to_agent, resp, strlen(resp)+1);
                                out = 1;
                            }
                        }
                    }
                    len = sizeof(msg);
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
    pthread_exit(NULL);
}
