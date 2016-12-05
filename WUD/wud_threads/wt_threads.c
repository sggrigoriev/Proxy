//
// Created by gsg on 01/12/16.
//
#include <wc_settings.h>
#include <string.h>
#include <errno.h>
#include <curl/curl.h>
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
static pu_queue_t* to_cloud;

//Total stop
static volatile int finish = 0;

//Queue events defines
typedef enum {WT_Timeout = PQ_TIMEOUT,
    WT_FromProxyQueue = 1,
    WT_FromMonitorQueue = 2,
    WT_FromWatchDogQueue = 3,
    WT_FromFWUpgrade = 4,
    WT_ToCloud = 5,
    WT_Size} wt_queue_events_t;


/** Thread attributes */
static pthread_t childrenReadThreadId;
static pthread_attr_t childrenReadThreadAttr;
static pthread_t monitorThreadId;
static pthread_attr_t monitorThreadAttr;
static pthread_t watchdogThreadId;
static pthread_attr_t watchdogThreadAttr;
static pthread_t fwupgradeThreadId;
static pthread_attr_t fwupgradeThreadAttr;
static pthread_t cloudWriteThreadId;
static pthread_attr_t cloudWriteThreadAttr;
//All local threads prototypes
static volatile int cl_stops = 1;
static void* children_read_thread(void* dummy_params);
static volatile int mon_stops = 1;
static void* monitor_thread(void* dummy_params);
static volatile int wd_stops = 1;
static void* watchdog_thread(void *dummy_params);
static volatile int fw_stops = 1;
static void* fwupgrade_thread(void *dummy_params);
static volatile int cr_stops = 1;
static void* cloud_write_thread(void *dummy_params);
////////////////////////////////////////////////////
//Run all child threads
static int routine_startup();
static void routine_shutdown();
static void stop_child_threads();
////////////////////////////////////////////////////
//Routine itself
//return 1 if OK, 0 if not
static pu_queue_msg_t q_msg[WC_MAX_MSG_LEN];
//static char answer[WC_MAX_MSG_LEN];
int wt_start_routine() {
    pu_queue_event_t events;

    if(!routine_startup()) {
        pu_log(LL_ERROR, "Routine thread initiation failed. Abort.");
        return 0;
    }

    events = pu_add_queue_event(pu_create_event_set(), WT_FromProxyQueue);
    events = pu_add_queue_event(events, WT_FromMonitorQueue);
    events = pu_add_queue_event(events, WT_FromWatchDogQueue);
    events = pu_add_queue_event(events, WT_FromFWUpgrade);
    events = pu_add_queue_event(events, WT_ToCloud);

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
    return 0;
}

//////////////////////////////////////////////////////////////////////////////
static int routine_startup() {
//cURL init
    if (curl_global_init(CURL_GLOBAL_ALL) != 0) {
        pu_log(LL_ERROR, "Error on cUrl initialiation. Something goes wrong. Exiting.");
        return 0;
    }
//Queues initiation
    pu_queues_init(WT_Size);
    from_proxy = pu_queue_create(wc_getQueuesRecAmt(), WT_FromProxyQueue);
    from_monitor = pu_queue_create(wc_getQueuesRecAmt(), WT_FromMonitorQueue);
    from_watchdog = pu_queue_create(wc_getQueuesRecAmt(), WT_FromWatchDogQueue);
    from_fw_upgrade = pu_queue_create(wc_getQueuesRecAmt(), WT_FromFWUpgrade);
    to_cloud = pu_queue_create(wc_getQueuesRecAmt(), WT_ToCloud);

    if(!from_proxy || !from_monitor || !from_watchdog || !from_fw_upgrade || !to_cloud) return 0;

    pu_log(LL_INFO, "Proxy main thread: initiated");

//Threads start
    pthread_attr_init(&childrenReadThreadAttr);
    if (pthread_create(&childrenReadThreadId, &childrenReadThreadAttr, &children_read_thread, NULL)) {
        pu_log(LL_ERROR, "[children_read_thread]: Creating write thread failed: %s", strerror(errno));
        return 0;
    }
    pu_log(LL_INFO, "[children_read_thread]: started.");

    pthread_attr_init(&monitorThreadAttr);
    if (pthread_create(&monitorThreadId, &monitorThreadAttr, &monitor_thread, NULL)) {
        pu_log(LL_ERROR, "[monitor_thread]: Creating write thread failed: %s", strerror(errno));
        return 0;
    }
    pu_log(LL_INFO, "[monitor_thread]: started.");

    pthread_attr_init(&watchdogThreadAttr);
    if (pthread_create(&watchdogThreadId, &watchdogThreadAttr, &watchdog_thread, NULL)) {
        pu_log(LL_ERROR, "[watchdog_thread]: Creating write thread failed: %s", strerror(errno));
        return 0;
    }
    pu_log(LL_INFO, "[watchdog_thread]: started.");

    pthread_attr_init(&fwupgradeThreadAttr);
    if (pthread_create(&fwupgradeThreadId, &fwupgradeThreadAttr, &fwupgrade_thread, NULL)) {
        pu_log(LL_ERROR, "[fwupgrade_thread]: Creating write thread failed: %s", strerror(errno));
        return 0;
    }
    pu_log(LL_INFO, "[fwupgrade_thread]: started.");

    pthread_attr_init(&cloudWriteThreadAttr);
    if (pthread_create(&cloudWriteThreadId, &cloudWriteThreadAttr, &cloud_write_thread, NULL)) {
        pu_log(LL_ERROR, "[cloud_write_thread]: Creating write thread failed: %s", strerror(errno));
        return 0;
    }
    pu_log(LL_INFO, "[cloud_write_thread]: started.");

    return 1;
}
static void routine_shutdown() {
//cURL stop
    curl_global_cleanup();
}
static void stop_child_threads() {

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
static void* cloud_write_thread(void *dummy_params) {
    pthread_exit(NULL);
}