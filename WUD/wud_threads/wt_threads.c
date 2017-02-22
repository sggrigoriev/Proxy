//
// Created by gsg on 01/12/16.
//

#include <string.h>
#include <errno.h>
#include <stdlib.h>

#include "pu_queue.h"
#include "pu_logger.h"
#include "pr_commands.h"

#include "wc_defaults.h"
#include "wc_settings.h"

#include "wm_childs_info.h"
#include <wa_manage_children.h>
#include <pr_commands.h>
#include "wf_upgrade.h"
#include "wa_alarm.h"
#include "wh_manager.h"



#include "wt_queues.h"
#include "wt_threads.h"

#include "wt_agent_proxy_read.h"
#include "wt_monitor.h"
#include "wt_watchdog.h"
#include "wt_fw_upgrade.h"
#include "wt_server_write.h"

#define PT_THREAD_NAME "WUD_MAIN"

////////////////////////////////////////////
// Global params
static pu_queue_t* to_main;
static pu_queue_t* to_cloud;

static pu_queue_msg_t q_msg[WC_MAX_MSG_LEN];

//Total stop
static volatile int stop = 0;

////////////////////////////////////////////////////
//Run all child threads except server_writer & fw_upgrade
static int routine_startup();
static void routine_shutdown();

static int run_fw_upgrade(pr_cmd_fwu_start_t fwu_start);
static void fw_upgrade_cancel();    //check if thread really works before

static int is_server_writer_run();
static int start_server_writer(pr_cmd_cloud_t conn_info);
static void stop_server_writer();    //check if thread really works before

static void process_command(pr_cmd_item_t cmd);
static void process_alert(pr_alert_item_t alert);
//////////////////////////////////////////////////////
//Main routine
////////////////////////////////////////////////////
int wt_request_processor() {

    if(!routine_startup()) {
        pu_log(LL_ERROR, "%s: request processor initiation failed. Abort.", PT_THREAD_NAME);
        return 0;
    }

    pu_queue_event_t events = pu_add_queue_event(pu_create_event_set(), WT_to_Main);

/*
    NB! The cloud should be notified. But before uncomment ths make suer the SERVER_WRITE will get connection info
    from Proxy!
    pr_make_wud_alert(&alert, PR_WUD_START, WC_DEFAULT_WUD_NAME);
    pu_queue_push(to_cloud, alert, strlen(alert)+1);  //Notify about the start
    free(alert);
*/
    pu_log(LL_INFO, "%s: start.", PT_THREAD_NAME);
/*
    if(!wf_was_download_empty()) {
        pu_log(LL_INFO, "%s: The download direcory was not empt during the startup. Alerted", PT_THREAD_NAME);
        pr_make_fwu_alert(&alert, PR_FWU_DOWNLOAD_BUSY, WC_DEFAULT_WUD_NAME);
        pu_queue_push(to_cloud, alert, strlen(alert)+1);
        free(alert);
        wf_set_download_state(WF_STATE_EMPTY);
    }
    if(!wf_was_upgrade_empty()) {
        pu_log(LL_INFO, "%s: The upgrade direcory was not empt during the startup. Alerted", PT_THREAD_NAME);
        pr_make_fwu_alert(&alert, PR_FWU_UPGRADE_BUSY, WC_DEFAULT_WUD_NAME);
        pu_queue_push(to_cloud, alert, strlen(alert)+1);
        free(alert);

        wf_set_download_state(WF_STATE_EMPTY);
    }
*/

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
                    msg_obj_t* item_obj = pr_parse_msg(q_msg);
                    if (!item_obj) {
                        pu_log(LL_ERROR, "%s: error parsing msg %s. Ignored", PT_THREAD_NAME, q_msg);
                        continue;
                    }
                    switch (pr_get_item_type(item_obj)) {
                        case PR_COMMANDS_MSG:
                            process_command(pr_get_cmd_item(item_obj));
                            break;
                        case PR_ALERTS_MSG: // special case! {"wud_ping": [{"deviceId":"gateway device id", "paramsMap":{"component":"zbagent"}}]
                            process_alert(pr_get_alert_item(item_obj));
                            break;
                        default:
                            pu_log(LL_ERROR, "%s: unsupported message %s. Ignored.", PT_THREAD_NAME, q_msg);
                            break;
                    }
                    pr_erase_msg(item_obj);
//restore original buf len for the next cycle
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

//////////////////////////////////////////////////////////////////////////////
static int routine_startup() {
//Queues initiation
    wt_init_queues();
    if(to_cloud = wt_get_gueue(WT_to_Cloud), !to_cloud) return 0;
    if(to_main = wt_get_gueue(WT_to_Main), !to_main) return 0;

//Threads start
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

    return 1;
}
static void routine_shutdown() {
    wt_set_stop_agent_proxy_read();
//    wt_set_stop_monitor();
    wt_set_stop_watchdog();
    wt_set_stop_fw_upgrade();
    wt_set_stop_server_write();

    wt_stop_agent_proxy_read();
//    wt_stop_monitor();
    wt_stop_watchdog();
    fw_upgrade_cancel();
    stop_server_writer();
}
//////////////////////////////////////////////////
static int fw_started = 0;
//
static int run_fw_upgrade(pr_cmd_fwu_start_t fwu_start) {
    pu_log(LL_DEBUG, "run_fw_upgrade - berinning");
    if(fw_started) return 1;    //Already run
    if(!wt_start_fw_upgrade(fwu_start)) return 0;
    fw_started = 1;
    pu_log(LL_INFO, "%s: fw_upgrade started", PT_THREAD_NAME);
    return 1;
}
static void fw_upgrade_cancel() {    //check if thread really works before
    if(!fw_started) return;
    wt_set_stop_fw_upgrade();
    wt_stop_fw_upgrade();
    fw_started = 0;
    pu_log(LL_INFO, "%s: fw_upgrade stop", PT_THREAD_NAME);
}
//////////////////////////////////////////////////
static int sw_started = 0;
//
static int is_server_writer_run() {
    return sw_started;
}
static int start_server_writer(pr_cmd_cloud_t conn_info) {
    wc_setURL(conn_info.conn_string);
    wc_setDeviceID(conn_info.device_id);
    wc_setAuthToken(conn_info.auth_token);
    wc_setFWVersion(conn_info.fw_version);
    wh_mgr_start();

    if(wt_start_server_write()) {
        sw_started = 1;
        pu_log(LL_INFO, "%s: server_writer start", PT_THREAD_NAME);
        return 1;
    }
    wh_mgr_stop();
    return 0;
}
static void stop_server_writer() {    //check if thread really works before
    if(!sw_started) return;
    wt_set_stop_server_write();
    wt_stop_server_write();
    wh_mgr_stop();
    pu_log(LL_INFO, "%s: server_writer stop", PT_THREAD_NAME);
}
//
static void process_command(pr_cmd_item_t cmd) {
    switch (cmd.command_type) {
        case PR_CMD_FWU_START:
            pu_log(LL_INFO, "%s: fw upgrade start command received. File server URL is %s", PT_THREAD_NAME, cmd.fwu_start.file_server_url);
            run_fw_upgrade(cmd.fwu_start);
            break;
        case PR_CMD_FWU_CANCEL:
            pu_log(LL_WARNING, "%s: fw upgrade cancellation command received", PT_THREAD_NAME);
            fw_upgrade_cancel();
            break;
        case PR_CMD_RESTART_CHILD:
            pu_log(LL_WARNING, "%s: %s restart requested", PT_THREAD_NAME, cmd.restart_child.component);
            if (!wa_restart_child(pr_string_2_chld(cmd.restart_child.component))) {
                pu_log(LL_ERROR, "%s: restart of %s failed. Reboot."PT_THREAD_NAME, cmd.restart_child.component);
                stop = 1;
            }
            else {
                char json[LIB_HTTP_MAX_MSG_SIZE];
                char di[LIB_HTTP_DEVICE_ID_SIZE];
                wc_getDeviceID(di, sizeof(di));
//TODO! currently there is no child restart - just reboot. Dear friend, change it sometime...
                pr_make_reboot_alert4cloud(json, sizeof(json), di);
                pu_queue_push(to_cloud, json, strlen(json)+1);  //Notifu cloud about the restart
            }
            break;
        case PR_CMD_CLOUD_CONN:
            pu_log(LL_INFO, "%s: Cloud connection info received", PT_THREAD_NAME);
            if(!is_server_writer_run()) {   //first run
                if (!start_server_writer(cmd.cloud_conn)) {
                    pu_log(LL_ERROR, "%s: server write thread start failed. Abort", PT_THREAD_NAME);
                    stop = 1;
                }
            }
            else {  //reconnect already running server_writed
                wh_reconnect(cmd.cloud_conn.conn_string, cmd.cloud_conn.auth_token);
            }
            break;
        case PR_CMD_STOP:
            pu_log(LL_WARNING, "%s: stop command received", PT_THREAD_NAME);
            stop = 1;
            break;
        default:
            pu_log(LL_ERROR, "%s: unsupported command type = %d received. Ignor.", PT_THREAD_NAME, cmd.command_type);
            break;
    }
}
static void process_alert(pr_alert_item_t alert) {
    switch(alert.alert_type) {
        case PR_ALERT_UNDEFINED:
            pu_log(LL_ERROR, "%s: Undefined alert type.Ignored", PT_THREAD_NAME);
            break;
        case PR_ALERT_WATCHDOG:
            pu_log(LL_INFO, "%s: watchdog message from %s", PT_THREAD_NAME, alert.alert_wd.component);
            wa_alarm_update(pr_string_2_chld(alert.alert_wd.component));
            break;
        case PR_ALERT_FWU_FAILED:
            pu_log(LL_WARNING, "%s Firware upgrade failed", PT_THREAD_NAME);
            fw_upgrade_cancel();    //Bad variant FWU fucked-uo
            break;
        case PR_ALERT_FWU_READY_4_INSTALL:   //fw file received, checked, moved & renamed - ready to start complition - reboot!
            pu_log(LL_INFO, "%s: fw upgrade: ready for install", PT_THREAD_NAME);
            fw_upgrade_cancel();    //just to clean-up after the process
            wa_stop_children();     //kill'em all and suicide after that
            stop = 1;
            break;
        default:
            pu_log(LL_ERROR, "%s: unsupported alert. Ignored.", PT_THREAD_NAME);
            break;
    }
}
