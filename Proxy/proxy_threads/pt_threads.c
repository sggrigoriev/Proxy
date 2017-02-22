//
// Created by gsg on 03/11/16.
//
#include <pthread.h>
#include <errno.h>
#include <memory.h>

#include "pr_commands.h"
#include "pf_traffic_proc.h"
#include <lib_timer.h>
#include <pf_cloud_conn_params.h>
#include <pf_proxy_commands.h>
#include <ph_manager.h>
#include <pr_commands.h>

#include "pc_defaults.h"
#include "pc_settings.h"
#include "pt_queues.h"
#include "pt_server_read.h"

#include "pt_threads.h"
#include "pt_server_write.h"
#include "pt_main_agent.h"

#ifndef PROXY_SEPARATE_RUN
#include "pt_wud_write.h"
#endif

#define PT_THREAD_NAME "MAIN_THREAD"

static int main_thread_startup();
static void main_thread_shutdown();
static void send_device_id_to_agent();
static void send_fw_version_to_cloud();
static void process_proxy_commands(const char* msg);

#ifndef PROXY_SEPARATE_RUN
static int initiate_wud();     //Send to WUD cloud connection info
static void send_wd();          // Send Watchdog to thw WUD
#endif
///////////////////////////////////////////////////////////////////////////////
//Main proxy thread
//
//The only main thread's buffer!
static pu_queue_msg_t mt_msg[PROXY_MAX_MSG_LEN];

static pu_queue_t* from_server;
static pu_queue_t* from_agent;
static pu_queue_t* to_server;
static pu_queue_t* to_agent;

#ifndef PROXY_SEPARATE_RUN
static pu_queue_t* to_wud;
lib_timer_clock_t wd_clock = {0};
#endif
lib_timer_clock_t cloud_url_update_clock = {0};
lib_timer_clock_t gw_fw_version_sending_clock = {0};
//TODO! If fw upgrade failed lock should be set to 0 again!!!
int lock_gw_fw_version_sending = 0;     //set to 1 when the fw upgrade starts.

static pu_queue_event_t events;

static volatile int main_finish;
/////////////////////////////////////////////////////////////////////////////////
//Main function
//
void pt_main_thread() { //Starts the main thread.

    main_finish = 0;

    if(!main_thread_startup()) {
        pu_log(LL_ERROR, "%s: Initialization failed. Abort", PT_THREAD_NAME);
        main_finish = 1;
    }
    unsigned int events_timeout = 0; //Wait until the end of univerce
#ifndef PROXY_SEPARATE_RUN
    events_timeout = 1;
    lib_timer_init(&wd_clock, pc_getProxyWDTO());   //Initiating the timer for watchdog sendings
#endif
    lib_timer_init(&cloud_url_update_clock, pc_getCloudURLTOHrs()*3600);    //Initiating the tomer for cloud URL request TO
    lib_timer_init(&gw_fw_version_sending_clock, pc_getFWVerSendToHrs()*3600);

    send_device_id_to_agent();  //sending the device id to agent
    send_fw_version_to_cloud(); //sending the fw version to the cloud
    while(!main_finish) {
        size_t len = sizeof(mt_msg);
        pu_queue_event_t ev;

        switch (ev=pu_wait_for_queues(events, events_timeout)) {
            case PS_Timeout:
//                pu_log(LL_DEBUG, "%s: TIMEOUT", PT_THREAD_NAME);
                break;
            case PS_FromServerQueue:
                while(pu_queue_pop(from_server, mt_msg, &len)) {
                    pu_log(LL_DEBUG, "%s: got command(s) array from server_read %s", PT_THREAD_NAME, mt_msg);
                    process_proxy_commands(mt_msg);
                    len = sizeof(mt_msg);
                }
                break;
            case PS_FromAgentQueue:
                while(pu_queue_pop(from_agent, mt_msg, &len)) {
                    pu_log(LL_DEBUG, "%s: got from agent_read %s", PT_THREAD_NAME, mt_msg);
                    pu_queue_push(to_server, mt_msg, len);
                    len = sizeof(mt_msg);
                }
                break;
            case PS_STOP:
                main_finish = 1;
                pu_log(LL_INFO, "%s received STOP event. Terminated", PT_THREAD_NAME);
                break;
            default:
                pu_log(LL_ERROR, "%s: Undefined event %d on wait (from agent / from server)!", PT_THREAD_NAME, ev);
                break;
        }
//Place for own actions
//1. Wathchdog
#ifndef PROXY_SEPARATE_RUN
        if(lib_timer_alarm(wd_clock)) {
            send_wd();
            lib_timer_init(&wd_clock, pc_getProxyWDTO());
        }
#endif
//2. Regular contact url update
        if(lib_timer_alarm(cloud_url_update_clock)) {
            pu_log(LL_INFO, "%s going to update the contact cloud URL...", PT_THREAD_NAME);
            ph_update_contact_url();
            lib_timer_init(&cloud_url_update_clock, pc_getCloudURLTOHrs()*3600);
        }
//3. Regular sending the fw version to the cloud
        if(lib_timer_alarm(gw_fw_version_sending_clock) && !lock_gw_fw_version_sending) {
            send_fw_version_to_cloud();
            lib_timer_init(&gw_fw_version_sending_clock, pc_getFWVerSendToHrs()*3600);
        }
    }
    main_thread_shutdown();
}
/////////////////////////////////////////////////////////////////////////////////
//Local functions
//
static int main_thread_startup() {
    init_queues();

    from_server = pt_get_gueue(PS_FromServerQueue);
    from_agent = pt_get_gueue(PS_FromAgentQueue);
    to_server = pt_get_gueue(PS_ToServerQueue);
    to_agent = pt_get_gueue(PS_ToAgentQueue);

#ifndef PROXY_SEPARATE_RUN
    to_wud = pt_get_gueue(PS_ToWUDQueue);
#endif

    events = pu_add_queue_event(pu_create_event_set(), PS_FromAgentQueue);
    events = pu_add_queue_event(events, PS_FromServerQueue);

    if(!start_server_read()) {
        pu_log(LL_ERROR, "%s: Creating %s failed: %s", PT_THREAD_NAME, "SERVER_READ", strerror(errno));
        return 0;
    }
    pu_log(LL_INFO, "%s: started", "SERVER_READ");

    if(!start_server_write()) {
        pu_log(LL_ERROR, "%s: Creating %s failed: %s", PT_THREAD_NAME, "SERVER_WRITE", strerror(errno));
        return 0;
    }
    pu_log(LL_INFO, "%s: started", "SERVER_WRITE");

    if(!start_agent_main()) {
        pu_log(LL_ERROR, "%s: Creating %s failed: %s", PT_THREAD_NAME, "AGENT_MAIN", strerror(errno));
        return 0;
    }
    pu_log(LL_INFO, "%s: started", "AGENT_MAIN");

#ifndef PROXY_SEPARATE_RUN
    if(!start_wud_write()) {
        pu_log(LL_ERROR, "%s: Creating %s failed: %s", PT_THREAD_NAME, "WUD_WRITE", strerror(errno));
        return 0;
    }
    pu_log(LL_INFO, "%s: started", "WUD_WRITE");

    return initiate_wud();
#else
    return 1;
#endif
}
static void main_thread_shutdown() {
    set_stop_server_read();
    set_stop_server_write();
    set_stop_agent_main();

#ifndef PROXY_SEPARATE_RUN
    set_stop_wud_write();
#endif

    stop_server_read();
    stop_server_write();
    stop_agent_main();

#ifndef PROXY_SEPARATE_RUN
    stop_wud_write();
#endif
    erase_queues();
}
#ifndef PROXY_SEPARATE_RUN
//Send to WUD cloud connection info
static int initiate_wud() {
    char json[LIB_HTTP_MAX_MSG_SIZE];
    char at[LIB_HTTP_AUTHENTICATION_STRING_SIZE];
    char url[LIB_HTTP_MAX_URL_SIZE];
    char di[LIB_HTTP_DEVICE_ID_SIZE];
    char ver[LIB_HTTP_FW_VERSION_SIZE];

    pc_getActivationToken(at, sizeof(at));
    pc_getCloudURL(url, sizeof(url));
    pc_getProxyDeviceID(di, sizeof(di));
    pc_getFWVersion(ver, sizeof(ver));

    pr_make_conn_info_cmd(json, sizeof(json), url, di, at, ver);
    pu_log(LL_DEBUG, "%s: gona send to WUD_WRITE %s", PT_THREAD_NAME, json);
    pu_queue_push(to_wud, json, strlen(json)+1);

    return 1;
}

// Send Watchdog to the WUD
static void send_wd() {
    char buf[LIB_HTTP_MAX_MSG_SIZE];
    char di[LIB_HTTP_DEVICE_ID_SIZE];

    pc_getProxyDeviceID(di, sizeof(di));
    pr_make_wd_alert4WUD(buf, sizeof(buf), pc_getProxyName(), di);

    pu_queue_push(to_wud, buf, strlen(buf)+1);
}

#endif
//Send DeviceID to the Agent
static void send_device_id_to_agent() {
    const char* first_msg_part = "{\"gw_gatewayDeviceId\":[{\"paramsMap\":{\"deviceId\":\"";
    const char* second_msg_part = "\"}}]}";

    char device_id[LIB_HTTP_DEVICE_ID_SIZE];
    char msg[LIB_HTTP_MAX_MSG_SIZE];
    pc_getProxyDeviceID(device_id, sizeof(device_id));

    snprintf(msg, sizeof(msg)-1, "%s%s%s", first_msg_part, device_id, second_msg_part);

    pu_queue_push(to_agent, msg, strlen(msg)+1);
    pu_log(LL_INFO, "%s: device id was sent to the Agent: %s", PT_THREAD_NAME, msg);

}
//Sending the fw version to the cloud accordinlgy to the schedule
static void send_fw_version_to_cloud() {
    char device_id[LIB_HTTP_DEVICE_ID_SIZE];
    char fw_ver[DEFAULT_FW_VERSION_SIZE];
    char msg[LIB_HTTP_MAX_MSG_SIZE];

    pc_getProxyDeviceID(device_id, sizeof(device_id));
    pc_getFWVersion(fw_ver, sizeof(fw_ver));

    pr_make_fw_status4cloud(msg, sizeof(msg), PR_FWU_STATUS_STOP, fw_ver, device_id);

    pu_queue_push(to_server, msg, strlen(msg+1));
    pu_log(LL_INFO, "%s: firmware version was sent to the Cloud: %s", PT_THREAD_NAME, msg);
}

static void process_proxy_commands(const char* msg) {
    msg_obj_t* cmd_array = pr_parse_msg(mt_msg);

    if(!cmd_array) {
        pu_log(LL_ERROR, "%s: wrong commands array structure in message %s. Ignored", PT_THREAD_NAME, msg);
        return;
    }
    size_t size = pr_get_array_size(cmd_array);
    if(!size) {
        pu_log(LL_WARNING, "%s: empty proxy commands array in message %s. Ignored", PT_THREAD_NAME, msg);
        pr_erase_msg(cmd_array);
        return;
    }
    for(size_t i = 0; i < size; i++) {
        msg_obj_t* cmd_arr_elem = pr_get_arr_item(cmd_array, i);
        pr_cmd_item_t cmd_item = pr_get_cmd_item(cmd_arr_elem);
        switch (cmd_item.command_type) {
#ifndef PROXY_SEPARATE_RUN
            case PR_CMD_FWU_START: {
                lock_gw_fw_version_sending = 1;

//            case PR_CMD_FWU_CANCEL:    // And who will initiate the cancellation???
                char for_wud[LIB_HTTP_MAX_MSG_SIZE];
                pr_obj2char(cmd_arr_elem, for_wud, sizeof(for_wud));
                pu_queue_push(to_wud, for_wud, strlen(for_wud)+1);
                break;
            }
            case PR_CMD_STOP:
                pu_log(LL_INFO, "%d: finished because of %s", PT_THREAD_NAME, msg);
                main_finish = 1;
#endif
            case PR_CMD_UPDATE_MAIN_URL:
                if(!ph_update_main_url(cmd_item.update_main_url.main_url)) {
                    pu_log(LL_ERROR, "%s: Main URL update failed", PT_THREAD_NAME);
                }
                break;
            case PR_CMD_UNDEFINED:
                pu_log(LL_ERROR, "%s: bad command syntax command %s in msg %s. Ignored.", PT_THREAD_NAME, cmd_arr_elem, msg);
                break;
            default:
                pu_log(LL_ERROR, "%d: unsopported command %s in message %s. Ignored", PT_THREAD_NAME, cmd_arr_elem, msg);
                break;
        }
    }
    pr_erase_msg(cmd_array);
}

