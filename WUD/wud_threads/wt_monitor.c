//
// Created by gsg on 07/12/16.
//
#include <pthread.h>
#include <string.h>
#include <errno.h>

#include "wt_queues.h"
#include "wc_defaults.h"
#include "wc_settings.h"
#include "wm_monitor.h"
#include "wt_monitor.h"

#define PT_THREAD_NAME "MONITOR"

//////////////////////////////////////////////////
static pthread_t id;
static pthread_attr_t attr;
static volatile int stop;

static void* monitor(void* params);
static int wakeup(unsigned int to);

int wt_start_monitor() {
    if(!wm_init_monitor_data()) {
        pu_log(LL_ERROR, "%s: Error Monitor data initiation", PT_THREAD_NAME);
        return 0;
    }
    if(pthread_attr_init(&attr)) return 0;
    if(pthread_create(&id, &attr, &monitor, NULL)) return 0;
    return 1;
}
void wt_set_stop_monitor() {
    stop = 1;
}
void wt_stop_monitor() {
    void *ret;

    wt_set_stop_monitor();
    pthread_join(id, &ret);
    pthread_attr_destroy(&attr);

    wn_destroy_monitor_data();
}
////////////////////////////////////////////////////
static unsigned int monitor_to;

static pu_queue_t* to_main;

static void* monitor(void* params) {
    monitor_to = wc_getWUDMonitoringTO();

    stop = 0;
    to_main = wt_get_gueue(WT_to_Main);

    while(!stop && wakeup(monitor_to)) {
        const char* msg = wm_monitor();
        if(msg) {   //Got smth to send to
            pu_queue_push(to_main, msg, strlen(msg) + 1);
            pu_log(LL_INFO, "%s: Alert %s sent to the Request Processor", PT_THREAD_NAME, msg);
        }
    }
    pu_log(LL_INFO, "%s is finished", PT_THREAD_NAME);
    pthread_exit(NULL);
}

static int wakeup(unsigned int to) {   // return 1 if time, return 0 if stop
    unsigned int ret = sleep(monitor_to);
    if(!ret) return 1;          // time to raise
    if(stop) return 0;     // some signal raized - chek he thread stop condition
    return wakeup(ret);           // no stop - let's sleep for the rest of time
}