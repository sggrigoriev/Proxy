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

#include "pt_queues.h"
#include "pu_logger.h"

#include "pc_defaults.h"
#include "pc_settings.h"
#include "ph_manager.h"
#include "pf_proxy_commands.h"

#include "pt_server_read.h"

#define PT_THREAD_NAME "SERVER_READ"

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
static pu_queue_t* to_agent;    /* Transport to agent write thread */

/*****************************************************************************************
 * Local functions
 */
/* Thread function */
static void* read_proc(void* params);

/* Send answer (ACK(s)) to the cloud if it sends command(s)
 *      msg - message as JSON object
 */
static void send_ack_respond(msg_obj_t* msg);

/* Get the message from the cloud
 *  buf     - buffer for message received
 *  size    - buffer size
 */
static void read_from_cloud(char* buf, size_t size);

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

static void* read_proc(void* params) {

    to_main = pt_get_gueue(PS_FromServerQueue);
    to_agent = pt_get_gueue(PS_ToAgentQueue);
    stop = 0;

    char buf[LIB_HTTP_MAX_MSG_SIZE];
    char proxy_id[LIB_HTTP_DEVICE_ID_SIZE];
    pc_getProxyDeviceID(proxy_id, sizeof(proxy_id));

/* Main read loop */
    while(!stop) {
        read_from_cloud(buf, sizeof(buf));
        pu_log(LL_DEBUG, "%s: received from cloud: %s", PT_THREAD_NAME, buf);
/* And hurray!! If the cloud sends command array to us we have to answer immediately! Promandablyadskayapizdoproushnaspizdorazjobannojrez'boy! */
        msg_obj_t* msg = pr_parse_msg(buf);
        if(!msg) {
            pu_log(LL_ERROR, "%s: Incoming message %s ignored", PT_THREAD_NAME, buf);
            continue;
        }
        if(pr_get_message_type(msg) != PR_COMMANDS_MSG) { /* currently we're not make business with alerts and/or measuruments in Proxy */
            pr_erase_msg(msg);
            pu_log(LL_INFO, "%s: message from cloud to Agent: %s", PT_THREAD_NAME, buf);
            pu_queue_push(to_main, buf, strlen(buf)+1);
        }
        else {      /* Here are commands! */
            char for_agent[LIB_HTTP_MAX_MSG_SIZE];
            char for_proxy[LIB_HTTP_MAX_MSG_SIZE];
            send_ack_respond(msg);
            pr_split_msg(msg, proxy_id, for_proxy, sizeof(for_proxy), for_agent, sizeof(for_agent));
            pr_erase_msg(msg);
            if(strlen(for_agent)) {
                pu_log(LL_INFO, "%s: from cloud to Agent: %s", PT_THREAD_NAME, for_agent);
                pu_queue_push(to_agent, for_agent, strlen(for_agent)+1);
            }
            if(strlen(for_proxy)) {
                pu_log(LL_INFO, "%s: command(s) array from cloud to Proxy: %s", PT_THREAD_NAME, for_proxy);
                pu_queue_push(to_main, for_proxy, strlen(for_proxy)+1);
            }
        }
    }
    pu_log(LL_INFO, "%s: STOP. Terminated", PT_THREAD_NAME);
    pthread_exit(NULL);
}

/************************************************************************************************
 * Local functions implementation
 */

static void send_ack_respond(msg_obj_t* msg) {
    char resp[LIB_HTTP_MAX_MSG_SIZE];
    char resp_to_resp[LIB_HTTP_MAX_MSG_SIZE] = {0};
    pf_answer_to_command(msg, resp, sizeof(resp));
    if(strlen(resp)) {
        if(!ph_respond(resp, resp_to_resp, sizeof(resp_to_resp))) {
            pu_log(LL_ERROR, "%s: Error responding. Reconnect", PT_THREAD_NAME);
            ph_reconnect();
        }
        else {
            pu_log(LL_INFO, "%s: Received from cloud back to the proxy command respond: %s", PT_THREAD_NAME, resp_to_resp);
        }
    }
}
static void read_from_cloud(char* buf, size_t size) {
    int out = 0;
    while(!out && !stop) {
        switch(ph_read(buf, size)) {
            case -1:        //error
                pu_log(LL_ERROR, "%s: Error reading. Reconnect", PT_THREAD_NAME);
                ph_reconnect();    /* loop again the succ inside */
                out = 0;
                break;
            case 0:     /* timeout - read again */
#ifdef PROXY_AUTH_GET_EMUL
                strcpy(buf, emu);
                    out = 1;
#endif
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