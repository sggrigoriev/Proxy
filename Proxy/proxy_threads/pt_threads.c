//
// Created by gsg on 03/11/16.
//
#include <pthread.h>
#include <errno.h>
#include <memory.h>
#include <pr_commands.h>
#include <pf_proxy_watchdog.h>
#include <pf_traffic_proc.h>

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
pf_clock_t clock = {0};
#endif

static pu_queue_event_t events;

void pt_main_thread() { //Starts the main thread.

    int main_finish = 0;


    if(!main_thread_startup()) {
        pu_log(LL_ERROR, "%s: Initialization failed. Abort");
        main_finish = 1;
    }

    while(!main_finish) {
        size_t len = sizeof(mt_msg);
        pu_queue_event_t ev;

        switch (ev=pu_wait_for_queues(events, DEFAULT_MAIN_THREAD_TO_SEC)) {
            case PS_Timeout:
                pu_log(LL_DEBUG, "%s: TIMEOUT", PT_THREAD_NAME);
                break;
            case PS_FromServerQueue:
                while(pu_queue_pop(from_server, mt_msg, &len)) {
                    pu_queue_push(to_agent, mt_msg, len);
                    pu_log(LL_DEBUG, "%s: msg %d bytes was sent to agent: %s ", PT_THREAD_NAME, len, mt_msg);
                    len = sizeof(mt_msg);
                }
                break;
            case PS_FromAgentQueue:
                while(pu_queue_pop(from_agent, mt_msg, &len)) {
                    len = pf_add_proxy_head(mt_msg, sizeof(mt_msg));
                    pu_queue_push(to_server, mt_msg, len);
                    pu_log(LL_DEBUG, "%s: msg %s was sent to server_write", PT_THREAD_NAME, mt_msg);
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
        if(pf_wd_time_to_send(clock)) send_wd();
#endif
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
#endif

    return 1;
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
    char *proxy_info;
    pr_message_t msg;
    char buf[PROXY_MAX_PATH];

    msg.message_type = PR_COMMAND;
    msg.command.cmd = PR_PRESTO_INFO;
    pc_getActivationToken(buf, sizeof(buf));
    msg.command.presto_info.activation_token = strdup(buf);

    pc_getCloudURL(buf, sizeof(buf));
    msg.command.presto_info.cloud_conn_string = strdup(buf);

    pc_getDeviceAddress(buf, sizeof(buf));
    msg.command.presto_info.device_id = strdup(buf);

    pr_make_message(&proxy_info, msg);
    pu_queue_push(to_wud, proxy_info, strlen(proxy_info)+1);

    pr_erase_msg(msg);
    free(proxy_info);

    return 1;
}

// Send Watchdog to the WUD
static void send_wd() {
    char *proxy_info;
    pr_message_t msg;

    msg.message_type = PR__WATCHDOG_ALERT;
    msg.watchdog_alert = strdup(pc_getProxyName());

    pr_make_message(&proxy_info, msg);
    pu_queue_push(to_wud, proxy_info, strlen(proxy_info)+1);

    pr_erase_msg(msg);
    free(proxy_info);
}

#endif