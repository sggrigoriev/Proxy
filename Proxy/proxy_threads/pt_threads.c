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

static void update_cloud_url();
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

static pu_queue_event_t events;

void pt_main_thread() { //Starts the main thread.

    int main_finish = 0;

    if(!main_thread_startup()) {
        pu_log(LL_ERROR, "%s: Initialization failed. Abort", PT_THREAD_NAME);
        main_finish = 1;
    }
#ifndef PROXY_SEPARATE_RUN
    lib_timer_init(&wd_clock, pc_getProxyWDTO());   //Initiating the timer for watchdog sendings
#endif
    lib_timer_init(&cloud_url_update_clock, pc_getCloudURLTOHrs()*3600);    //Initiating the tomer for cloud URL request TO

    while(!main_finish) {
        size_t len = sizeof(mt_msg);
        pu_queue_event_t ev;

        switch (ev=pu_wait_for_queues(events, DEFAULT_MAIN_THREAD_TO_SEC)) {
            case PS_Timeout:
                pu_log(LL_DEBUG, "%s: TIMEOUT", PT_THREAD_NAME);
                break;
            case PS_FromServerQueue:
                while(pu_queue_pop(from_server, mt_msg, &len)) {
                    pu_log(LL_DEBUG, "%s: got from server_read %s", PT_THREAD_NAME, mt_msg);
                    pf_cmd_t* pf_cmd = pf_parse_cloud_commands(mt_msg);
                    if(pf_cmd) {
                        char resp[PROXY_MAX_MSG_LEN];
                        pu_queue_push(to_server, pf_answer_to_command(resp, sizeof(resp), mt_msg), strlen(resp)+1);
                    }
                    if(pf_are_proxy_commands(pf_cmd)) pf_process_proxy_commands(pf_cmd);
                    pu_queue_push(to_agent, mt_msg, len);
                    pf_close_cloud_commands(pf_cmd);
                    len = sizeof(mt_msg);
                }
                break;
            case PS_FromAgentQueue:
                while(pu_queue_pop(from_agent, mt_msg, &len)) {
                    pu_log(LL_DEBUG, "%s: got from agent_read %s", PT_THREAD_NAME, mt_msg);
                    pu_queue_push(to_server, mt_msg, len); //TODO mlevitin
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
        if(lib_timer_alarm(wd_clock)) { send_wd(); lib_timer_init(&wd_clock, pc_getProxyWDTO()); }
#endif
        if(lib_timer_alarm(cloud_url_update_clock)) { ph_update_contact_url(); lib_timer_init(&cloud_url_update_clock, pc_getCloudURLTOHrs()*3600); }
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

    pc_getActivationToken(at, sizeof(at));
    pc_getCloudURL(url, sizeof(url));
    pc_getProxyDeviceID(di, sizeof(di));

    pr_make_cmd_cloud(url, di, at, json, sizeof(json));

    pu_queue_push(to_wud, json, strlen(json)+1);

    return 1;
}

// Send Watchdog to the WUD
static void send_wd() {
    char json[LIB_HTTP_MAX_MSG_SIZE];
    char pn[PROXY_MAX_PROC_NAME];
    char di[LIB_HTTP_DEVICE_ID_SIZE];

    pc_getProxyDeviceID(di, sizeof(di));
    strncpy(pn, pc_getProxyName(), sizeof(pn)-1);
    pr_make_wd_alert(pn, json, sizeof(json), di, 11041);

    pu_queue_push(to_wud, json, strlen(json)+1);
}

#endif
