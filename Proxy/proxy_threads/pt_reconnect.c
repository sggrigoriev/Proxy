/*
 *  Copyright 2018 People Power Company
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
 Created by gsg on 29/05/19.
*/

#include <pthread.h>
#include <string.h>
#include <pr_commands.h>

#include "pu_logger.h"
#include "pt_queues.h"
#include "lib_http.h"
#include "pf_traffic_proc.h"

#include "pc_settings.h"
#include "ph_manager.h"

#include "pt_reconnect.h"

#define PT_THREAD_NAME "CHANGE_MAIN_URL"

static const char* good = "Main URL updated";
static const char* bad = "Main URL remains the same. Update failed";

static pthread_t id;
static pthread_attr_t attr;
static volatile int stop = 1;

static pu_queue_t* to_cloud;    /* Send answer to the command */
static pu_queue_t* to_main;     /* Send the result to main thread, just to notify the end of process */

static char new_url[LIB_HTTP_MAX_URL_SIZE]={'\0'};
static unsigned long commandID = 0L;

static void send_reboot() {
    char buf[LIB_HTTP_MAX_MSG_SIZE] = {0};
    char deviceID[LIB_HTTP_DEVICE_ID_SIZE] = {0};

    pc_getProxyDeviceID(deviceID, sizeof(deviceID));
    pu_log(LL_INFO, "%s: cloud rejects the Proxy connection - reboot", PT_THREAD_NAME);
    fprintf(stdout, "%s: cloud rejects the Proxy connection - reboot\n", PT_THREAD_NAME);
    pr_make_reboot_command(buf, sizeof(buf), deviceID);
    pu_queue_push(to_main, buf, strlen(buf) + 1);
}

static void send_rc_to_cloud(unsigned long cmd_id, t_pf_rc rc) {
    char buf[LIB_HTTP_MAX_MSG_SIZE]={0};
    char device_id[LIB_HTTP_DEVICE_ID_SIZE];

    pc_getProxyDeviceID(device_id, sizeof(device_id));
    pf_make_answer_to_command(cmd_id, buf, sizeof(buf), rc);
    pf_add_proxy_head(buf, sizeof(buf), device_id);
    pu_queue_push(to_cloud, buf, strlen(buf)+1);
}

static void* thread_proc(void* params) {
    stop = 0;
    pu_log(LL_INFO, "%s: started", PT_THREAD_NAME);
    to_cloud = pt_get_gueue(PS_ToServerQueue);
    to_main = pt_get_gueue(PS_FromReconnectQueue);
    const char* answer;

    switch(ph_update_main_url(new_url)) {
        case 1:
            send_rc_to_cloud(commandID, PF_RC_OK);
            answer = good;
            break;
        case -1:
            send_reboot(); /* Currently should not work for M3 */
        default: /* If got 0 somehow + same reaction for -1 */
            pu_log(LL_ERROR, "%s: Main URL update failed", PT_THREAD_NAME);
            send_rc_to_cloud(commandID, PF_RC_FMT_ERR);
            answer = bad;
            break;
    }
    pu_queue_push(to_main, answer, strlen(answer)+1);
    sleep(3600);        /* To wait external killer */
    stop = 1;
    pthread_exit(NULL);
}

int start_reconnect(const char* main_url, unsigned long cmd_id) {
    strncpy(new_url, main_url, sizeof(new_url)-1);
    commandID =  cmd_id;

    if(pthread_attr_init(&attr)) return 0;
    if(pthread_create(&id, &attr, &thread_proc, NULL)) return 0;
    return 1;
}
void kill_reconnect() {
    void *ret;

    if(stop) return;

    pthread_cancel(id);
    pthread_join(id, &ret);
    pthread_attr_destroy(&attr);
    pu_log(LL_INFO, "%s killed. RIP", PT_THREAD_NAME);
}

