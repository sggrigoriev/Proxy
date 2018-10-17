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
    Created by gsg on 03/11/16.
*/
#include <pthread.h>
#include <errno.h>
#include <memory.h>
#include <pf_traffic_proc.h>
#include <proxy_functions/pf_proxy_commands.h>
#include <pr_commands.h>

#include "lib_timer.h"
#include "pt_queues.h"
#include "lib_tcp.h"
#include "pr_commands.h"

#include "pc_defaults.h"
#include "pc_settings.h"
#include "ph_manager.h"
#include "pt_server_read.h"
#include "pt_server_write.h"
#include "pt_main_agent.h"
#include "pt_wud_write.h"

#include "pt_threads.h"


#define PT_THREAD_NAME "MAIN_THREAD"

/*******************************************************************************
 * Local functions
 */
static int main_thread_startup();                           /* Total Proxy therads startup */
static void main_thread_shutdown();                         /* Total Proxy shutdown */
static void send_fw_version_to_cloud();                     /* Inform the cloud about the gateway firmware version */
static void send_reboot_status(pr_reboot_param_t status);   /* Sand the alert to the cloud: bofore reboot and after reboot */
static void process_cloud_message(char* cloud_msg);         /* Parse the info from the clud and sends ACKs to Agent and/or provede commands to the Proxy */
static void process_agent_message(char* msg);               /* Parse & split Agent's messages to rge cloud and to the Proxy; forward cloud and process Proxy's */
static void process_proxy_commands(char* msg);              /* Process Proxy commands - commsnd(s) string as input */
static void report_cloud_conn_status(int online);           /* send off/on line status notification to the Agent, sent conn info to the WUD if online */
static void send_wd();                                      /* Send Watchdog to thw WUD */
static void send_reboot();                                  /* Send reboot request to WUD */
/***********************************************************************************************
 * Main Proxy thread data
 */
static pu_queue_msg_t mt_msg[PROXY_MAX_MSG_LEN];    /* The only main thread's buffer! */

static pu_queue_t* from_server;     /* server_read -> main_thread */
static pu_queue_t* from_agent;      /* agent_read -> main_thread */

static pu_queue_t* to_server;       /* main_thread -> server_write */
static pu_queue_t* to_agent;        /* main_thread -> agent_write */
static pu_queue_t* to_wud;          /* main_thread -> wud_write */

lib_timer_clock_t wd_clock = {0};           /* timer for watchdog sending */
lib_timer_clock_t cloud_url_update_clock = {0};         /*  timer for contact URL request sending */
lib_timer_clock_t gw_fw_version_sending_clock = {0};    /* timer for firmware version info sending */
/*TODO! If fw upgrade failed lock should be set to 0 again!!! */
int lock_gw_fw_version_sending = 0;     /* set to 1 when the fw upgrade starts. */

int proxy_is_online = 0;                /* 0 - no connection to the cloud; 1 - got connection to the cloud */

static pu_queue_event_t events;         /* main thread events set */

static volatile int main_finish;        /* stop flag for main thread */
/*********************************************************************************************
 * Public function implementation
 */
void pt_main_thread() { /* Starts the main thread. */
    main_finish = 0;
    int agent_got_online_status = 0;

    if(!main_thread_startup()) {
        pu_log(LL_ERROR, "%s: Initialization failed. Abort", PT_THREAD_NAME);
        main_finish = 1;
    }
    lib_timer_init(&wd_clock, pc_getProxyWDTO());   /* Initiating the timer for watchdog sendings */
    lib_timer_init(&cloud_url_update_clock, pc_getCloudURLTOHrs()*3600);        /* Initiating the tomer for cloud URL request TO */
    lib_timer_init(&gw_fw_version_sending_clock, pc_getFWVerSendToHrs()*3600);

    report_cloud_conn_status(0);  /* sending to the agent offline status - no connection with the cloud */
    send_fw_version_to_cloud();                 /* sending the fw version to the cloud */
    send_reboot_status(PR_AFTER_REBOOT);        /* sending the reboot status to the cloud */

    pu_log(LL_ERROR, "%s: Main loop starts", PT_THREAD_NAME);
    while(!main_finish) {
        size_t len = sizeof(mt_msg);
        pu_queue_event_t ev;

        switch (ev=pu_wait_for_queues(events, DEFAULT_S_TO)) {
            case PS_Timeout:
                break;
            case PS_FromServerQueue:
                while(pu_queue_pop(from_server, mt_msg, &len)) {
                    pu_log(LL_DEBUG, "%s: got message from the cloud %s", PT_THREAD_NAME, mt_msg);
                    process_cloud_message(mt_msg);
                    len = sizeof(mt_msg);
                }
                break;
            case PS_FromAgentQueue:
                while(pu_queue_pop(from_agent, mt_msg, &len)) {
                    pu_log(LL_DEBUG, "%s: got message from the Agent %s", PT_THREAD_NAME, mt_msg);
                    process_agent_message(mt_msg);
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
/* Place for own periodic actions */
/*1. Wathchdog */
        if(lib_timer_alarm(wd_clock)) {
            send_wd();
            lib_timer_init(&wd_clock, pc_getProxyWDTO());
        }
/*2. Regular contact url update */
        if(lib_timer_alarm(cloud_url_update_clock)) {
            pu_log(LL_INFO, "%s going to update the contact cloud URL...", PT_THREAD_NAME);
            proxy_is_online = 0;
            report_cloud_conn_status(proxy_is_online);
            if((ph_update_contact_url() == -1) && (pc_rebootIfCloudRejects()))
                send_reboot();
            else {
                proxy_is_online = 1;
                report_cloud_conn_status(proxy_is_online);

                lib_timer_init(&cloud_url_update_clock, pc_getCloudURLTOHrs() * 3600);
            }
        }
/* 3. Regular sending the fw version to the cloud */
        if(lib_timer_alarm(gw_fw_version_sending_clock) && !lock_gw_fw_version_sending) {
            send_fw_version_to_cloud();
            lib_timer_init(&gw_fw_version_sending_clock, pc_getFWVerSendToHrs()*3600);
        }
    }
    main_thread_shutdown();
}

/***************************************************************************************************
 * Local functions implementation
 */
static int main_thread_startup() {
    init_queues();

    from_server = pt_get_gueue(PS_FromServerQueue);
    from_agent = pt_get_gueue(PS_FromAgentQueue);
    to_server = pt_get_gueue(PS_ToServerQueue);
    to_agent = pt_get_gueue(PS_ToAgentQueue);
    to_wud = pt_get_gueue(PS_ToWUDQueue);

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

    if(!start_wud_write()) {
        pu_log(LL_ERROR, "%s: Creating %s failed: %s", PT_THREAD_NAME, "WUD_WRITE", strerror(errno));
        return 0;
    }
    pu_log(LL_INFO, "%s: started", "WUD_WRITE");

    return 1;
}

static void main_thread_shutdown() {
    set_stop_server_read();
    set_stop_server_write();
    set_stop_agent_main();
    set_stop_wud_write();

    stop_server_read();
    stop_server_write();
    stop_agent_main();
    stop_wud_write();

    erase_queues();
}

/* Send Watchdog to the WUD */
static void send_wd() {
        char buf[LIB_HTTP_MAX_MSG_SIZE];
        char di[LIB_HTTP_DEVICE_ID_SIZE];

        pc_getProxyDeviceID(di, sizeof(di));
        pr_make_wd_alert4WUD(buf, sizeof(buf), pc_getProxyName(), di);

        pu_queue_push(to_wud, buf, strlen(buf)+1);
    }

/*Sending the fw version to the cloud accordingly to the schedule */
static void send_fw_version_to_cloud() {
    char device_id[LIB_HTTP_DEVICE_ID_SIZE];
    char fw_ver[DEFAULT_FW_VERSION_SIZE];
    char msg[LIB_HTTP_MAX_MSG_SIZE];

    pc_getProxyDeviceID(device_id, sizeof(device_id));
    pc_getFWVersion(fw_ver, sizeof(fw_ver));

    pr_make_fw_status4cloud(msg, sizeof(msg), PR_FWU_STATUS_STOP, fw_ver, device_id);
    pf_add_proxy_head(msg, sizeof(msg), device_id);

    pu_queue_push(to_server, msg, strlen(msg)+1);
    pu_log(LL_INFO, "%s: firmware version was sent to the Cloud: %s", PT_THREAD_NAME, msg);
}

static void send_reboot_status(pr_reboot_param_t status) {
    char device_id[LIB_HTTP_DEVICE_ID_SIZE];
    char msg[LIB_HTTP_MAX_MSG_SIZE];

    pc_getProxyDeviceID(device_id, sizeof(device_id));

    pr_make_reboot_alert4cloud(msg, sizeof(msg), device_id, status);
    pf_add_proxy_head(msg, sizeof(msg), device_id);
    pu_queue_push(to_server, msg, strlen(msg)+1);
    pu_log(LL_INFO, "%s: reboot status was sent to the Cloud: %s", PT_THREAD_NAME, msg);
}

static void process_cloud_message(char* cloud_msg) {
    msg_obj_t* obj = pr_parse_msg(cloud_msg);
    if(!obj) {
        pu_log(LL_ERROR, "%s: Incoming message %s ignored", PT_THREAD_NAME, cloud_msg);
        return;
    }
    if(pr_get_message_type(obj) != PR_COMMANDS_MSG) { /* currently we're not make business with alerts and/or measuruments in Proxy */
        pr_erase_msg(obj);
        pu_log(LL_INFO, "%s: message from cloud to Agent: %s", PT_THREAD_NAME, cloud_msg);
        pu_queue_push(to_agent, cloud_msg, strlen(cloud_msg)+1);
    }
    else {      /* Here are commands! */
        char for_agent[LIB_HTTP_MAX_MSG_SIZE]={0};
        char for_proxy[LIB_HTTP_MAX_MSG_SIZE]={0};

        char device_id[LIB_HTTP_DEVICE_ID_SIZE];

        pc_getProxyDeviceID(device_id, sizeof(device_id));

/* Separate the info berween Proxy & Agent */
/* NB! Currently the Proxy eats commands only! */
        pr_split_msg(obj, device_id, for_proxy, sizeof(for_proxy), for_agent, sizeof(for_agent));
        pr_erase_msg(obj);
        if(strlen(for_agent)) { /* If there is smth for Agent, lets send to him all the shit */
            pu_log(LL_INFO, "%s: from cloud to Agent: %s", PT_THREAD_NAME, for_agent);
            pu_queue_push(to_agent, cloud_msg, strlen(cloud_msg)+1);
        }
        if(strlen(for_proxy)) {
            pu_log(LL_INFO, "%s: command(s) array from cloud to Proxy: %s", PT_THREAD_NAME, for_proxy);
            process_proxy_commands(for_proxy);
        }
    }
}
static void process_agent_message(char* msg) {
    msg_obj_t* obj = pr_parse_msg(msg);
    if(!obj) {
        pu_log(LL_ERROR, "%s: Incoming message %s ignored", PT_THREAD_NAME, msg);
        return;
    }
    if(pr_agent_connected(obj)) {              /* Got the request from Agent - he is connected and functioning, Hurray! */
        char buf[LIB_HTTP_MAX_MSG_SIZE] = {0};
        char deviceID[LIB_HTTP_DEVICE_ID_SIZE] = {0};
        char auth_token[LIB_HTTP_AUTHENTICATION_STRING_SIZE] = {0};
        char conn_string[LIB_HTTP_MAX_URL_SIZE] = {0};

/* Send status + deviceID + connInfo (by request from Agent */
        pc_getProxyDeviceID(deviceID, sizeof(deviceID));
        pc_getAuthToken(auth_token, sizeof(auth_token));
        pc_getMainCloudURL(conn_string, sizeof(conn_string));

        pr_conn_state_notf_to_agent(buf, sizeof(buf), deviceID, proxy_is_online, auth_token, conn_string);
        pu_queue_push(to_agent, buf, strlen(buf)+1);
    }
    else {  /* The rest of messages from the Agent are for cloud */
        pu_queue_push(to_server, msg, strlen(msg)+1);
    }
    pr_erase_msg(obj);
}

static void send_rc_to_cloud(msg_obj_t* cmd, t_pf_rc rc) {
    char buf[LIB_HTTP_MAX_MSG_SIZE]={0};
    char device_id[LIB_HTTP_DEVICE_ID_SIZE];

    pc_getProxyDeviceID(device_id, sizeof(device_id));
    pf_answer_to_command(cmd, buf, sizeof(buf), rc);
    pf_add_proxy_head(buf, sizeof(buf), device_id);
    pu_queue_push(to_server, buf, strlen(buf)+1);
}
/*
 * Responce to the command; Process the command;
 */
static void process_proxy_commands(char* msg) {
    msg_obj_t* cmd_array = pr_parse_msg(msg);

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
    size_t i;
    for(i = 0; i < size; i++) {
        msg_obj_t* cmd_arr_elem = pr_get_arr_item(cmd_array, i);    /* Get Ith command */
        pr_cmd_item_t cmd_item = pr_get_cmd_item(cmd_arr_elem);     /* Get params of Ith command */

        switch (cmd_item.command_type) {
            case PR_CMD_FWU_START: {
                lock_gw_fw_version_sending = 1;
                char for_wud[LIB_HTTP_MAX_MSG_SIZE];
                msg_obj_t* cmd_array = pr_make_cmd_array(cmd_arr_elem);
                pr_obj2char(cmd_array, for_wud, sizeof(for_wud));
                pu_queue_push(to_wud, for_wud, strlen(for_wud)+1);
                pr_erase_msg(cmd_array);
                break;
            }
            case PR_CMD_STOP:
                pu_log(LL_INFO, "%s: finished because of %s", PT_THREAD_NAME, msg);
                main_finish = 1;
            case PR_CMD_UPDATE_MAIN_URL:
                proxy_is_online = 0;
                report_cloud_conn_status(proxy_is_online);
                switch(ph_update_main_url(cmd_item.update_main_url.main_url)) {
                    case 1:
                        break;
                    case -1:
                        if(pc_rebootIfCloudRejects()) send_reboot();
                    default: /* -1, 0 */
                        pu_log(LL_ERROR, "%s: Main URL update failed", PT_THREAD_NAME);
                        send_rc_to_cloud(cmd_arr_elem, PF_RC_EXEC_ERR);
                        break;
                }
                proxy_is_online = 1;
                report_cloud_conn_status(proxy_is_online);
                send_rc_to_cloud(cmd_arr_elem, PF_RC_OK);
                break;
            case PR_CMD_REBOOT: {    /* The cloud kindly asks to shut up & reboot */
                pu_log(LL_INFO, "%s: CLoud command REBOOT received", PT_THREAD_NAME);
                char for_wud[LIB_HTTP_MAX_MSG_SIZE];
                msg_obj_t* cmd_array = pr_make_cmd_array(cmd_arr_elem);
                pr_obj2char(cmd_array, for_wud, sizeof(for_wud));
                pu_queue_push(to_wud, for_wud, strlen(for_wud) + 1);
                pr_erase_msg(cmd_array);
                break;
            }
            case PR_CMD_CLOUD_CONN: /* reconnection case */
                if(!proxy_is_online) {  /* proxy was offline and now got new connecion! */
                    pu_log(LL_INFO, "%s: Proxy connected with the cloud", PT_THREAD_NAME);
                    proxy_is_online = 1;
                    report_cloud_conn_status(proxy_is_online);
                }
                else {
                    pu_log(LL_INFO, "%s: Secondary connection alert. Ignored", PT_THREAD_NAME);
                }
                break;
            case PR_CMD_CLOUD_OFF:
                if(proxy_is_online) {   /* connection disapeared - has to notify about it */
                    pu_log(LL_INFO, "%s: Proxy disconnected with the cloud", PT_THREAD_NAME);
                    proxy_is_online = 0;
                    report_cloud_conn_status(proxy_is_online);
                }
                else {
                    pu_log(LL_INFO, "%s: Secondary disconnection alert. Ignored", PT_THREAD_NAME);
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

static void report_cloud_conn_status(int online) {
    char buf[LIB_HTTP_MAX_MSG_SIZE] = {0};
    char deviceID[LIB_HTTP_DEVICE_ID_SIZE] = {0};
    char auth_token[LIB_HTTP_AUTHENTICATION_STRING_SIZE] = {0};
    char conn_string[LIB_HTTP_MAX_URL_SIZE] = {0};

/* 1. Send the alert to the Agent */
    pc_getProxyDeviceID(deviceID, sizeof(deviceID));
    pc_getAuthToken(auth_token, sizeof(auth_token));
    pc_getMainCloudURL(conn_string, sizeof(conn_string));

    pr_conn_state_notf_to_agent(buf, sizeof(buf), deviceID, online, auth_token, conn_string);
    pu_queue_push(to_agent, buf, strlen(buf)+1);

/* 2. if the status == online - send the connection info to the WUD */
    if(online) {
        char fw_version[LIB_HTTP_FW_VERSION_SIZE] = {0};
        char local_ip_address[LIB_HTTP_MAX_IPADDRES_SIZE] = {0};
        char interface[LIB_HTTP_MAX_IP_INTERFACE_SIZE] = {0};

        pc_getCloudURL(conn_string, sizeof(conn_string));
        pc_getFWVersion(fw_version, sizeof(fw_version));

        pr_make_conn_info_cmd(buf, sizeof(buf), conn_string, deviceID, auth_token, fw_version);
        pu_queue_push(to_wud, buf, strlen(buf)+1);
/* 3. Send the local IP-address to the cloud. */
#ifdef PROXY_ETHERNET_INTERFACE
        strncpy(interface, PROXY_ETHERNET_INTERFACE, sizeof(interface)-1);
#endif
        lib_tcp_local_ip(interface, local_ip_address, sizeof(local_ip_address));
        if(strlen(local_ip_address)) {
            pr_make_local_ip_notification(buf, sizeof(buf), local_ip_address, deviceID);
            pf_add_proxy_head(buf, sizeof(buf), deviceID);
            pu_queue_push(to_server, buf, strlen(buf)+1);
        }
        else {
            pu_log(LL_WARNING, "report_cloud_conn_status: No info about local IP Address. Check PROXY_ETHERNET_INTERFACE define in Make file!");
        }
    }
}

static void send_reboot() {
    char buf[LIB_HTTP_MAX_MSG_SIZE] = {0};
    char deviceID[LIB_HTTP_DEVICE_ID_SIZE] = {0};

    pc_getProxyDeviceID(deviceID, sizeof(deviceID));
    pu_log(LL_INFO, "%s: cloud rejects the Proxy connection - reboot");
    pr_make_reboot_command(buf, sizeof(buf), deviceID);
    to_wud = pt_get_gueue(PS_ToWUDQueue);
    pu_queue_push(to_wud, buf, strlen(buf) + 1);
}
