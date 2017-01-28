//
// Created by gsg on 01/12/16.
//

#include <string.h>
#include <errno.h>
#include <curl/curl.h>

#include <wt_http_utl.h>
#include <pr_commands.h>
#include <wm_childs_info.h>
#include <wa_manage_children.h>
#include <wf_upgrade.h>
#include <stdlib.h>
#include <wa_alarm.h>

#include "pu_queue.h"
#include "pu_logger.h"
#include "wc_defaults.h"
#include "wc_settings.h"

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
//Run all child threads
static int routine_startup();
static void routine_shutdown();
static int run_fw_upgrade(const char* conn_string);
static void fw_upgrade_cancel();    //check if thread really works before
static int run_server_writer();
static void stop_server_writer();    //check if thread really works before
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
                    pr_alert_t alert;
                    pr_json_2_struct(&alert, q_msg);
                    switch(alert.alert_type) {
                        case PR_ERROR_ALERT:
                            pu_log(LL_ERROR, "%s: parsing message.Ignored - %s", PT_THREAD_NAME, alert.error.error_text);
                            break;
                        case PR_MONITOR_ALERT:
                            pu_log(LL_WARNING, "%s: monitor alert received %s", PT_THREAD_NAME, q_msg);
                            pu_queue_push(to_cloud, q_msg, len);   //Notify cloud
                            break;
                        case PR_WATCHDOG_ALERT:
                            pu_log(LL_INFO, "%s: watchdog message from %s", PT_THREAD_NAME, alert.watchdog.component);
                            wa_alarm_update(pr_string_2_chld(alert.watchdog.component));
                             break;
                        case PR_FWU_ALERT:
                            pu_log(LL_WARNING, "%s: fw upgrade alert received %s ", PT_THREAD_NAME, q_msg);
                            pu_queue_push(to_cloud, q_msg, len);    //Notify cloud
                            switch(alert.fwu.fwu_alert_type) {
                                case PR_FWU_INIT_FAILED:        //In all these cases we know the process is over!
                                case PR_FWU_DOWNLOAD_FAIL:
                                case PR_FWU_CHECK_FAIL:
                                case PR_FWU_MOVE_FAIL:
                                case PR_FWU_CANCEL:
                                case PR_FWU_MOVE_OK:
                                    fw_upgrade_cancel();    //just to clean-up after the process
                                    break;
                                default:
                                    break;
                            }
                            break;
                        case PR_COMMAND:
                            switch (alert.cmd_type) {
                                case PR_CMD_FWU_START:
                                    pu_log(LL_INFO, "%s: fw upgrade start command received. File server URL is %s", PT_THREAD_NAME, alert.cmd_fwu.file_server_url);
                                    run_fw_upgrade(alert.cmd_fwu.file_server_url);
                                    break;
                                case PR_CMD_FWU_CANCEL:
                                    pu_log(LL_WARNING, "%s: fw upgrade cancellation command received", PT_THREAD_NAME);
                                    fw_upgrade_cancel();
                                    break;
                                case PR_CMD_RESTART_CHILD:
                                    pu_log(LL_WARNING, "%s: %s restart requested", PT_THREAD_NAME, alert.cmd_restart.component);
                                    if (!wa_restart_child(pr_string_2_chld(alert.cmd_restart.component))) {
                                        pu_log(LL_ERROR, "%s: restart of %s failed. Reboot."PT_THREAD_NAME, alert.cmd_restart.component);
                                        stop = 1;
                                    }
                                    else {
                                        char json[LIB_HTTP_MAX_MSG_SIZE];
                                        char di[LIB_HTTP_DEVICE_ID_SIZE];
                                        wc_getDeviceID(di, sizeof(di));
                                        pr_make_wd_alert(alert.cmd_restart.component, di, 11050, json, sizeof(json));
                                        pu_queue_push(to_cloud, json, strlen(json)+1);  //Notifu cloud about the restart
                                    }
                                    break;
                                case PR_CMD_CLOUD_CONN:
                                    pu_log(LL_INFO, "%s: Cloud connection info received", PT_THREAD_NAME);
                                    wt_set_connection_info(alert.cmd_cloud.conn_string, alert.cmd_cloud.device_id, alert.cmd_cloud.auth_token);
                                    if (!run_server_writer()) { //it could be already run - the op will be skip
                                        pu_log(LL_ERROR, "%s: server write thread start failed. Abort", PT_THREAD_NAME);
                                        stop = 1;
                                    }
                                    break;
                                case PR_CMD_STOP:
                                    pu_log(LL_WARNING, "%s: stop command received", PT_THREAD_NAME);
                                    stop = 1;
                                    break;
                                default:
                                    pu_log(LL_ERROR, "%s: unsupported command type = %d received. Ignor.", PT_THREAD_NAME, alert.cmd_type);
                                    break;
                            }
                            break;
                        default:
                            pu_log(LL_ERROR, "%s: unsupported alert type = %d received. Ignor.", PT_THREAD_NAME, alert.alert_type);
                            break;
                    }

                }
                break;
             default:
                pu_log(LL_ERROR, "%s: Unsupported queue event %d on wait!",PT_THREAD_NAME, ev);
                break;
        }
        len = sizeof(q_msg);    //restore original buf len for the next cycle
    }
    routine_shutdown();
    pu_log(LL_INFO, "%s is finished", PT_THREAD_NAME);
    return 0;
}

//////////////////////////////////////////////////////////////////////////////
static int routine_startup() {
//cURL init
    if(!wt_http_curl_init()) {
        pu_log(LL_ERROR, "%s: Error on cUrl initialiation. Something goes wrong. Exiting.", PT_THREAD_NAME);
        return 0;
    }
//Queues initiation
    wt_init_queues();
    if(to_cloud = wt_get_gueue(WT_to_Cloud), !to_cloud) return 0;

//Threads start
    if(!wt_start_agent_proxy_read()) {
        pu_log(LL_ERROR, "%s: start agent/proxy read thread failed: %s", PT_THREAD_NAME, strerror(errno));
        return 0;
    }
    pu_log(LL_INFO, "%s: Agent/Proxy read thread started.", PT_THREAD_NAME);

    if(!wt_start_monitor()) {
        pu_log(LL_ERROR, "%s: start monitoring thread failed: %s", PT_THREAD_NAME, strerror(errno));
        return 0;
    }
    pu_log(LL_INFO, "%s: monitoring thread started.", PT_THREAD_NAME);

    if(!wt_start_watchdog()) {
        pu_log(LL_ERROR, "%s: start witchdog thread failed: %s", PT_THREAD_NAME, strerror(errno));
        return 0;
    }
    pu_log(LL_INFO, "%s: watchdog thread started.", PT_THREAD_NAME);

    return 1;
}
static void routine_shutdown() {
//cURL stop
    wt_http_curl_stop();

    wt_set_stop_agent_proxy_read();
    wt_set_stop_monitor();
    wt_set_stop_watchdog();
    wt_set_stop_fw_upgrade();
    wt_set_stop_server_write();

    wt_stop_agent_proxy_read();
    wt_stop_monitor();
    wt_stop_watchdog();
    fw_upgrade_cancel();
    stop_server_writer();
}
//////////////////////////////////////////////////
static int fw_started = 0;
//
static int run_fw_upgrade(const char* conn_string) {
    if(fw_started) return 1;    //Already run
    if(!wt_start_fw_upgrade(conn_string)) return 0;
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
static int run_server_writer() {
    if(sw_started) return 1;
    if(!wt_start_server_write()) return 0;
    sw_started = 1;
    pu_log(LL_INFO, "%s: server_writer start", PT_THREAD_NAME);
    return 1;
}
static void stop_server_writer() {    //check if thread really works before
    if(!sw_started) return;
    wt_set_stop_server_write();
    wt_stop_server_write();
    pu_log(LL_INFO, "%s: server_writer stop", PT_THREAD_NAME);
}
