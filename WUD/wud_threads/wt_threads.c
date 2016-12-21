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

#include "pu_queue.h"
#include "pu_logger.h"
#include "wc_defaults.h"
#include "wc_settings.h"

#include "wt_queues.h"
#include "wt_threads.h"

#include "ww_watchdog.h"
#include "wt_agent_proxy_read.h"
#include "wt_monitor.h"
#include "wt_watchdog.h"
#include "wt_fw_upgrade.h"
#include "wt_server_write.h"

#define PT_THREAD_NAME "WUD_MAIN"

////////////////////////////////////////////
// Global params
static pu_queue_t* from_proxy;
static pu_queue_t* from_monitor;
static pu_queue_t* from_watchdog;
static pu_queue_t* from_fw_upgrade;
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
    pu_queue_event_t events;
    char* alert;

    if(!routine_startup()) {
        pu_log(LL_ERROR, "%s: request processor initiation failed. Abort.", PT_THREAD_NAME);
        return 0;
    }

    events = pu_add_queue_event(pu_create_event_set(), WT_ProxyAgentMessage);
    events = pu_add_queue_event(events, WT_WatchDogQueue);
    events = pu_add_queue_event(events, WT_MonitorAlerts);
    events = pu_add_queue_event(events, WT_FWInfo);

/*
    NB! The cloud should be notofied. But before uncomment ths make suer the SERVER_WRITE will get connection info
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

    pu_log(LL_DEBUG, "%s: from proxy queue = %lu", PT_THREAD_NAME, from_proxy);
    pu_log(LL_DEBUG, "%s: from_monitor queue = %lu", PT_THREAD_NAME, from_monitor);
    pu_log(LL_DEBUG, "%s: from_watchdog queue = %lu", PT_THREAD_NAME, from_watchdog);
    pu_log(LL_DEBUG, "%s: from_fw_upgrade queue = %lu", PT_THREAD_NAME, from_fw_upgrade);
    pu_log(LL_DEBUG, "%s: to_cloud queue = %lu", PT_THREAD_NAME, to_cloud);
    while(!stop) {
        pu_queue_event_t ev;
        size_t len = sizeof(q_msg);

        switch (ev=pu_wait_for_queues(events, 0)) {
            case WT_Timeout:
                break;
            case WT_ProxyAgentMessage: {
                while(pu_queue_pop(from_proxy, q_msg, &len)) {
                    pu_log(LL_DEBUG, "%s: received from Proxy/Agent: %s", PT_THREAD_NAME, q_msg);
                    pr_message_t msg = pr_json_2_msg(q_msg);
                    switch(msg.msg_type) {
                        case PR_MSG_ERROR:
                            pu_log(LL_ERROR, "%s: parsing message from proxy/agent.Ignored - %s", PT_THREAD_NAME, msg.err.err_msg);
                            break;
                        case PR_WATCHDOG_ALERT:
                            pu_log(LL_INFO, "%s: watchdog message from %s", PT_THREAD_NAME, msg.watchdog_alert.process_name);
                            ww_watchdog_update(msg.watchdog_alert.process_name);
                            break;
                        case PR_COMMAND:
                            switch(msg.cmd_type.cmd) {
                                case PR_FW_UPGRADE_START:
                                    pu_log(LL_INFO, "%s: fw upgrade start command received - %s", PT_THREAD_NAME, msg.fw_start.file_server_url);
                                    pr_make_fwu_alert(&alert, PR_FWU_INIT_OK, WC_DEFAULT_WUD_NAME);
                                    pu_queue_push(to_cloud, alert, strlen(alert)+1);   //Notify cloud
                                    run_fw_upgrade(msg.fw_start.file_server_url);
                                    break;
                                case PR_FW_UPGRADE_CANCEL:
                                    pu_log(LL_WARNING, "%s: fw upgrade cancellation command received", PT_THREAD_NAME);
                                    fw_upgrade_cancel();
//TODO: Mate Cancel alert from the wt_fw_upgrade!
                                    pr_make_fwu_alert(&alert, PR_FWU_CANCEL, WC_DEFAULT_WUD_NAME);
                                    pu_queue_push(to_cloud, alert, strlen(alert)+1);   //Notify cloud
                                    break;
                                case PR_STOP:   //Strange case...Proxy wants to be shut with all of processes
                                    pu_log(LL_WARNING, "%s: Presto stop command received", PT_THREAD_NAME);
                                    stop = 1;
                                    break;
                                case PR_PRESTO_INFO:    //Proxy sends the conn string for communications with cloud & own deviceID (same purpose)
                                    pu_log(LL_INFO, "%s: Cloud connection info received", PT_THREAD_NAME);
                                    wt_set_connection_info(msg.presto_info.cloud_conn_string, msg.presto_info.device_id, msg.presto_info.activation_token);
                                    if (!run_server_writer()) {
                                        pu_log(LL_ERROR, "%s: server write thread start failed. Abort", PT_THREAD_NAME);
                                        stop = 1;
                                    }
                                    break;
                                default:
                                    pu_log(LL_ERROR, "%s: Proxy/Agent sent unsupported command type = %d. Ignored", PT_THREAD_NAME, msg.cmd_type.cmd);
                                    break;
                            }
                            break;
                        default:
                            pu_log(LL_ERROR, "%s: Proxy/Agent sent unsupported message type = %d. Ignored", PT_THREAD_NAME, msg.msg_type);
                            break;
                    }
                    pr_erase_msg(msg);
                    len = sizeof(q_msg);    //restore original buf len for the next cycle
                }
                break;
            }
            case WT_MonitorAlerts:
                while(pu_queue_pop(from_monitor, q_msg, &len)) {
                    pu_log(LL_DEBUG, "%s: received from Monitor: %s", PT_THREAD_NAME, q_msg);
                    pr_message_t msg = pr_json_2_msg(q_msg);
                    switch(msg.msg_type) {
                        case PR_MSG_ERROR:
                            pu_log(LL_ERROR, "%s: parsing message from monitor. Ignored - %s", PT_THREAD_NAME, msg.err.err_msg);
                            break;
                        case PR_MONITOR_ALERT:
                            pu_log(LL_WARNING, "%s: alert received from monitor %s", PT_THREAD_NAME, q_msg);
                            pu_queue_push(to_cloud, q_msg, len);   //Notify cloud
                            break;
                        case PR_COMMAND:
                            switch(msg.cmd_type.cmd) {
                                case PR_STOP:
                                    pu_log(LL_WARNING, "%s: stop command received from monitor", PT_THREAD_NAME);
                                    stop = 1;
                                    break;
                                case PR_RESTART_CHILD:
                                    pu_log(LL_WARNING, "%s: %s restart requested by monitor", PT_THREAD_NAME, pr_chld_2_string(msg.restart_child.process_idx));
                                    pu_queue_push(to_cloud, q_msg, len);   //Notify cloud
                                    if (!wa_restart_child(msg.restart_child.process_idx)) {
                                        pu_log(LL_ERROR, "%s: restart of %s failed. Reboot."PT_THREAD_NAME, pr_chld_2_string(msg.restart_child.process_idx));
                                        stop = 1;
                                    }
                                    break;
                                default:
                                    pu_log(LL_ERROR, "%s: Monitor sent unsupported message type: %d", PT_THREAD_NAME, msg.cmd_type.cmd);
                                    break;
                            }
                            break;
                        default:
                            pu_log(LL_ERROR, "%s: Monitor sent unsupported message type: %d", PT_THREAD_NAME, msg.msg_type);
                            break;
                    }
                    pr_erase_msg(msg);
                    len = sizeof(q_msg);    //restore original buf len for the next cycle
                }
                break;
            case WT_WatchDogQueue:
                while(pu_queue_pop(from_watchdog, q_msg, &len)) {
                    pu_log(LL_DEBUG, "%s: received from Watchdog: %s", PT_THREAD_NAME, q_msg);
                    pr_message_t msg = pr_json_2_msg(q_msg);
                    switch(msg.msg_type) {
                        case PR_MSG_ERROR:
                            pu_log(LL_ERROR, "%s: parsing message from watchdog. Ignored - %s", PT_THREAD_NAME, msg.err.err_msg);
                            break;
                        case PR_COMMAND:
                            switch(msg.cmd_type.cmd) {
                                case PR_RESTART_CHILD:
                                    pu_log(LL_INFO, "%s: restart request for %s from watchdog", PT_THREAD_NAME, pr_chld_2_string(msg.restart_child.process_idx));
                                    pu_queue_push(to_cloud, q_msg, len);   //Notify cloud
                                    if (!wa_restart_child(msg.restart_child.process_idx)) {
                                        pu_log(LL_ERROR, "%s: restart of %s failed. Reboot."PT_THREAD_NAME, pr_chld_2_string(msg.restart_child.process_idx));
                                        stop = 1;
                                    }
                                    break;
                                default:
                                    pu_log(LL_ERROR, "%s: Watchdog sent unsupported command %d", PT_THREAD_NAME, msg.cmd_type.cmd);
                                    break;
                            }
                        default:
                            pu_log(LL_ERROR, "%s: Watchdog sent unsupported message type: %d", PT_THREAD_NAME, msg.msg_type);
                            break;
                    }
                    pr_erase_msg(msg);
                    len = sizeof(q_msg);    //restore original buf len for the next cycle
                }
                break;
            case WT_FWInfo:     //On case the fw_upgrade process works
                while(pu_queue_pop(from_fw_upgrade, q_msg, &len)) {
                    pu_log(LL_DEBUG, "%s: received from FW Upgrade: %s", PT_THREAD_NAME, q_msg);
                    pr_message_t msg = pr_json_2_msg(q_msg);
                    switch(msg.msg_type) {
                        case PR_MSG_ERROR:
                            pu_log(LL_ERROR, "%s: patsing message from fw upgrade. Ignored - %s", PT_THREAD_NAME, msg.err.err_msg);
                            break;
                        case PR_FWU_ALERT:
                            pu_log(LL_WARNING, "%s: alert %s received from fw upgrade", PT_THREAD_NAME, q_msg);
                            pu_queue_push(to_cloud, q_msg, len);    //Notify cloud
                            switch(msg.fw_upgrade_alert.alert_type) {
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
                        default:
                            pu_log(LL_ERROR, "%s: fw upgrade sent unsupported message type: %d", PT_THREAD_NAME, msg.msg_type);
                            break;
                    }
                    pr_erase_msg(msg);
                    len = sizeof(q_msg);    //restore original buf len for the next cycle
                }
                break;
             default:
                pu_log(LL_ERROR, "%s: Undefined event %d on wait!",PT_THREAD_NAME, ev);
                break;
        }
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
    from_proxy = wt_get_gueue(WT_ProxyAgentMessage);
    from_watchdog = wt_get_gueue(WT_WatchDogQueue);
    from_monitor = wt_get_gueue(WT_MonitorAlerts);
    from_fw_upgrade = wt_get_gueue(WT_FWInfo);
    to_cloud = wt_get_gueue(WT_AlertsToCloud);

    if(!from_proxy || !from_monitor || !from_watchdog || !from_fw_upgrade || !to_cloud) return 0;

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
