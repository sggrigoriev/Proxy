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
    Created by gsg on 01/12/16.
*/

#include <string.h>
#include <errno.h>

#include "pu_queue.h"
#include "pu_logger.h"
#include "pr_commands.h"

#include "wc_defaults.h"
#include "wc_settings.h"

#include "wa_manage_children.h"
#include "wa_alarm.h"
#include "wh_manager.h"

#include "wt_queues.h"
#include "wt_threads.h"

#include "wt_agent_proxy_read.h"
#include "wt_watchdog.h"
#include "wt_fw_upgrade.h"
#include "wt_server_write.h"

#define PT_THREAD_NAME "WUD_MAIN"

/*******************************************
 * Local data
 */
static pu_queue_t* to_main;         /* Queue to receive messages */
static pu_queue_t* to_cloud;        /* Queue to send messages to server_write (to cloud) */

static pu_queue_msg_t q_msg[WC_MAX_MSG_LEN];    /* Buffer for messages received */

static volatile int stop = 0;       /* request processor stop flag */
static int fw_started = 0;          /* firmware upgrade start sign */

/*******************************************
 * Local functiones definition
 */

/*******************************************
 * Run all child threads except server_writer & fw_upgrade
 * @return  - 1 if OK, 0 if not
 */
static int routine_startup();

/*******************************************
 * Stop all running threads
 */
static void routine_shutdown();

/*******************************************
 * Start firmware upgrade thread
 * @param fwu_start     - thread start parameters (URL, check sum)
 * @return  - 1 if OK, o if not
 */
static int run_fw_upgrade(pr_cmd_fwu_start_t fwu_start);

/*******************************************
 * Stop firmware upgrade thread. check if thread really works before
 */
static void fw_upgrade_cancel();

/*******************************************
 * Process the message sent to WUD (WD, alert ...
 * @param msg - the message in JSON formet
 */
static void process_message(char* msg);

/*******************************************
 * Process commands array arrived
 * @param cmd   - JSON array
 */
static void process_commands(msg_obj_t* cmd);

/*******************************************
 * Send reboot alert to the cloud and reboot the gateway
 */
static void process_reboot();

/*******************************************
 * Process alerts array arrived
 * @param alert     - alerts array in internal presentation
 */
static void process_alerts(msg_obj_t* cmd);

/********************************************************
    Main routine
*/
int wt_request_processor() {

    if(!routine_startup()) {
        pu_log(LL_ERROR, "%s: request processor initiation failed. Abort.", PT_THREAD_NAME);
        return 0;
    }

    pu_queue_event_t events = pu_add_queue_event(pu_create_event_set(), WT_to_Main);

    pu_log(LL_INFO, "%s: start.", PT_THREAD_NAME);

    pu_log(LL_DEBUG, "%s: to_main queue = %lu", PT_THREAD_NAME, to_main);
    pu_log(LL_DEBUG, "%s: to_cloud queue = %lu", PT_THREAD_NAME, to_cloud);
    while(!stop) {
        pu_queue_event_t ev;
        size_t len = sizeof(q_msg);

        switch (ev=pu_wait_for_queues(events, 0)) {
            case WT_Timeout:
                break;
            case WT_to_Main:
                while(pu_queue_pop(to_main, q_msg, &len)) {
                    pu_log(LL_DEBUG, "%s: received: %s", PT_THREAD_NAME, q_msg);
                    process_message(q_msg);
/* restore original buf len for the next cycle */
                    len = sizeof(q_msg);
                }
                break;
             default:
                pu_log(LL_ERROR, "%s: Unsupported queue event %d on wait!",PT_THREAD_NAME, ev);
                break;
        }
    }
    routine_shutdown();
    pu_log(LL_INFO, "%s is finished", PT_THREAD_NAME);
    return 0;
}

/********************************************************
 * Local functions implementation
 */

static int routine_startup() {
/* Queues initiation */
    wt_init_queues();
    if(to_cloud = wt_get_gueue(WT_to_Cloud), !to_cloud) return 0;
    if(to_main = wt_get_gueue(WT_to_Main), !to_main) return 0;

/* Threads start */
    if(!wt_start_agent_proxy_read()) {
        pu_log(LL_ERROR, "%s: start agent/proxy read thread failed: %s", PT_THREAD_NAME, strerror(errno));
        return 0;
    }
    pu_log(LL_INFO, "%s: Agent/Proxy read thread started.", PT_THREAD_NAME);
/*
    if(!wt_start_monitor()) {
        pu_log(LL_ERROR, "%s: start monitoring thread failed: %s", PT_THREAD_NAME, strerror(errno));
        return 0;
    }
    pu_log(LL_INFO, "%s: monitoring thread started.", PT_THREAD_NAME);
*/
    if(!wt_start_watchdog()) {
        pu_log(LL_ERROR, "%s: start witchdog thread failed: %s", PT_THREAD_NAME, strerror(errno));
        return 0;
    }
    pu_log(LL_INFO, "%s: watchdog thread started.", PT_THREAD_NAME);

    if (!wt_start_server_write()) {
        pu_log(LL_ERROR, "%s: server write thread start failed. Abort", PT_THREAD_NAME);
        return 0;
    }
    pu_log(LL_INFO, "%s: server write thread started.", PT_THREAD_NAME);
    return 1;
}

static void routine_shutdown() {
    wt_set_stop_agent_proxy_read();
/*    wt_set_stop_monitor(); */
    wt_set_stop_watchdog();
    wt_set_stop_fw_upgrade();
    wt_set_stop_server_write();

    wt_stop_agent_proxy_read();
/*     wt_stop_monitor(); */
    wt_stop_watchdog();
    fw_upgrade_cancel();
    wt_stop_server_write();
}

static int run_fw_upgrade(pr_cmd_fwu_start_t fwu_start) {
    if(fw_started) return 1;    /* Already run */
    if(!wt_start_fw_upgrade(fwu_start)) return 0;
    fw_started = 1;
    pu_log(LL_INFO, "%s: fw_upgrade started", PT_THREAD_NAME);
    return 1;
}

static void fw_upgrade_cancel() {
    if(!fw_started) return;
    wt_set_stop_fw_upgrade();
    wt_stop_fw_upgrade();
    fw_started = 0;
    pu_log(LL_INFO, "%s: fw_upgrade stop", PT_THREAD_NAME);
}

static void process_message(char* msg) {
    msg_obj_t* obj = pr_parse_msg(msg);
    if (!obj) {
        pu_log(LL_ERROR, "%s: error parsing msg %s. Ignored", PT_THREAD_NAME, msg);
        return;
    }
    switch (pr_get_message_type(obj)) {
        case PR_COMMANDS_MSG:
            process_commands(obj);
            break;
        case PR_ALERTS_MSG: /* special cases! "wud_ping" and gw_cloud_connections */
            process_alerts(obj);
            break;
        default:
            pu_log(LL_ERROR, "%s: unsupported message %s. Ignored.", PT_THREAD_NAME, msg);
            break;
    }
    pr_erase_msg(obj);
}

static void process_commands(msg_obj_t* cmds) {

    cJSON* cmd_array = pr_get_cmd_array(cmds);
    size_t size = pr_get_array_size(cmd_array);
    if(!size) {
        char* string = cJSON_PrintUnformatted(cmds);
        pu_log(LL_WARNING, "%s: empty proxy commands array: %s. Ignored", PT_THREAD_NAME, string);
        free(string);
        return;
    }
    size_t i;
    for(i = 0; i < size; i++) {
        msg_obj_t* cmd_arr_elem = pr_get_arr_item(cmd_array, i);
        pr_cmd_item_t cmd_item = pr_get_cmd_item(cmd_arr_elem);
        switch (cmd_item.command_type) {
            case PR_CMD_FWU_START:
                pu_log(LL_INFO, "%s: fw upgrade start command received. File server URL is %s", PT_THREAD_NAME, cmd_item.fwu_start.file_server_url);
                run_fw_upgrade(cmd_item.fwu_start);
                break;
            case PR_CMD_FWU_CANCEL:
                pu_log(LL_WARNING, "%s: fw upgrade cancellation command received", PT_THREAD_NAME);
                fw_upgrade_cancel();
                break;
            case PR_CMD_RESTART_CHILD:
                pu_log(LL_WARNING, "%s: %s restart requested", PT_THREAD_NAME, cmd_item.restart_child.component);
                if (!wa_restart_child(pr_string_2_chld(cmd_item.restart_child.component))) {
                    pu_log(LL_ERROR, "%s: restart of %s failed. Reboot.", PT_THREAD_NAME, cmd_item.restart_child.component);
                    process_reboot();
                }
                break;
            case PR_CMD_REBOOT: {
                pu_log(LL_WARNING, "%s: REBOOT requested", PT_THREAD_NAME);
                process_reboot();
            }
                break;
            case PR_CMD_CLOUD_CONN:
                pu_log(LL_INFO, "%s: Cloud connection info received", PT_THREAD_NAME);
                wc_setURL(cmd_item.cloud_conn.conn_string);
                wc_setDeviceID(cmd_item.cloud_conn.device_id);
                wc_setAuthToken(cmd_item.cloud_conn.auth_token);
                wc_setFWVersion(cmd_item.cloud_conn.fw_version);
                wh_reconnect();
                break;
            case PR_CMD_STOP:
                pu_log(LL_WARNING, "%s: stop command received", PT_THREAD_NAME);
                stop = 1;
                break;
            default:
                pu_log(LL_ERROR, "%s: unsupported command type = %d received. Ignor.", PT_THREAD_NAME, cmd_item.command_type);
                break;
        }
    }
}
static int reboot_was_sent = 0; /* Workaround for multiple child restarts */
static void process_reboot() {
    char json[LIB_HTTP_MAX_MSG_SIZE];
    char di[LIB_HTTP_DEVICE_ID_SIZE];
    if(reboot_was_sent) return;
    reboot_was_sent = 1;
    wc_getDeviceID(di, sizeof(di));

    pr_make_reboot_alert4cloud(json, sizeof(json), di, PR_BEFORE_REBOOT);
    pu_queue_push(to_cloud, json, strlen(json)+1);  /* Notify cloud about the restart */
    sleep(5);   /* Just give time to send the reboot notification to the cloud */
    stop = 1;
}

static void process_alerts(msg_obj_t* alerts) {
    cJSON* alerts_array = pr_get_alerts_array(alerts);
    size_t size = pr_get_array_size(alerts_array);
    if(!size) {
        char* string = cJSON_PrintUnformatted(alerts);
        pu_log(LL_WARNING, "%s: empty proxy alerts array: %s. Ignored", PT_THREAD_NAME, string);
        free(string);
        return;
    }
    size_t i;
    for(i = 0; i < size; i++) {
        msg_obj_t *alert_arr_elem = pr_get_arr_item(alerts_array, i);
        pr_alert_item_t alert = pr_get_alert_item(alert_arr_elem);

        switch(alert.alert_type) {
            case PR_ALERT_UNDEFINED:
                pu_log(LL_ERROR, "%s: Undefined alert type. Ignored", PT_THREAD_NAME);
                break;
            case PR_ALERT_WATCHDOG:
                pu_log(LL_INFO, "%s: watchdog message from %s", PT_THREAD_NAME, alert.alert_wd.component);
                wa_alarm_update(pr_string_2_chld(alert.alert_wd.component));
                break;
            case PR_ALERT_FWU_FAILED:
                pu_log(LL_WARNING, "%s Firware upgrade failed", PT_THREAD_NAME);
                fw_upgrade_cancel();    /* Bad variant FWU fucked-uo */
                break;
            case PR_ALERT_FWU_READY_4_INSTALL:   /* fw file received, checked, moved & renamed - ready to start complition - reboot! */
                pu_log(LL_INFO, "%s: fw upgrade: ready for install", PT_THREAD_NAME);
                fw_upgrade_cancel();    /* just to clean-up after the process */
                process_reboot();
                stop = 1;
                break;
            default:
                pu_log(LL_ERROR, "%s: unsupported alert. Ignored.", PT_THREAD_NAME);
                break;
        }
    }
}
