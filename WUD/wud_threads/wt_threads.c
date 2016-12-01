//
// Created by gsg on 01/12/16.
//
#include "pu_queue.h"
#include "pu_logger.h"
#include "wc_defaults.h"
#include "wf_upgrade.h"
#include "wu_http_post.h"
#include "wm_monitor.h"
#include "ww_watchdog.h"
#include "wt_threads.h"

////////////////////////////////////////////
// Global params
static pu_queue_t* from_proxy;
static pu_queue_t* from_monitor;
static pu_queue_t* from_watchdog;
static pu_queue_t* from_fw_upgrade;

//Total stop
static volatile int finish = 0;

//Queue events defines
typedef enum {WT_Timeout = PQ_TIMEOUT,  WT_FromProxyQueue = 1, WT_FromMonitorQueue = 2, WT_FromWatchDogQueue = 3, WT_FromFWUpgrade = 4, WT_Size} wt_queue_events_t;


/** Thread attributes */
static pthread_t childrenReadThreadId;
static pthread_attr_t monitorThreadAttr;
static pthread_t watchdogThreadId;
static pthread_attr_t fwupgradeThreadAttr;
//All local threads prototypes
static volatile int cl_stops = 1;
static void* children_read_thread(void* dummy_params);
static volatile int mon_stops = 1;
static void* monitor_thread(void* dummy_params);
static volatile int wd_stops = 1;
static void* watchdog_thread(void *dummy_params);
static volatile int fw_stops = 1;
static void* fwupgrade_thread(void *dummy_params);
////////////////////////////////////////////////////
//Run all child threads
static int routine_startup();
static void routine_shutdown();
////////////////////////////////////////////////////
//Routine itself
//return 1 if OK, 0 if not
static pu_queue_msg_t q_msg[WC_MAX_MSG_LEN];
//static char answer[WC_MAX_MSG_LEN];
int wt_start_routine() {
    pu_queue_event_t* events;

    if(!routine_startup()) {
        pu_log(LL_ERROR, "Routine thread initiation failed. Abort.");
        return 0;
    }

    events = pu_create_event_set();
    pu_add_queue_event(events, WT_FromProxyQueue);
    pu_add_queue_event(events, WT_FromMonitorQueue);
    pu_add_queue_event(events, WT_FromWatchDogQueue);
    pu_add_queue_event(events, WT_FromFWUpgrade);

    while(!finish) {
        pu_queue_event_t ev;
        size_t len;

        switch (ev=pu_wait_for_queues(events, 0)) {
            case WT_Timeout:
                break;
            case WT_FromProxyQueue: {
                if(!pu_queue_pop(from_proxy, q_msg, &len)) {
                    pu_log(LL_WARNING, "Routune: Empty message from proxy.");
                    break;
                }
                wf_upgrade(q_msg);
                break;
            }
            case WT_FromMonitorQueue:
                if(!pu_queue_pop(from_monitor, q_msg, &len)) {
                    pu_log(LL_WARNING, "Routune: Empty message from monitor.");
                    break;
                }
                wm_monitor(q_msg);
                break;
            case WT_FromWatchDogQueue:
                if(!pu_queue_pop(from_watchdog, q_msg, &len)) {
                    pu_log(LL_WARNING, "Routune: Empty message from watchdog.");
                    break;
                }
                ww_watchdog(q_msg);
                break;
            case WT_FromFWUpgrade:
                break;
            default:
                pu_log(LL_ERROR, "wt_start_routine: Undefined event %d on wait!", ev);
                break;
        }
    }
    routine_shutdown();
    pu_delete_event_set(events);
    return 0;
}

//////////////////////////////////////////////////////////////////////////////
static int routine_startup() {
    return 0;
}
static void routine_shutdown() {
}
////////////////////////////////////////////////////////////////////////////////
//Threads
static void* children_read_thread(void* dummy_params) {
    pthread_exit(NULL);
}

static void* monitor_thread(void* dummy_params) {
    pthread_exit(NULL);
}
static void* watchdog_thread(void *dummy_params) {
    pthread_exit(NULL);
}
static void* fwupgrade_thread(void *dummy_params) {
    pthread_exit(NULL);
}