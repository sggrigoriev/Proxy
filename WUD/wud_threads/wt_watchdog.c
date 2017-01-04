//
// Created by gsg on 08/12/16.
//
#include <pthread.h>
#include <string.h>
#include <stdlib.h>

#include "pu_logger.h"
#include "wc_settings.h"
#include "wt_queues.h"
#include "ww_watchdog.h"

#include "wt_watchdog.h"

#define PT_THREAD_NAME "WATCHDOG"

////////////////////////////
static pthread_t id;
static pthread_attr_t attr;
static volatile int stop;

static pu_queue_t* to_rp;

static void* wd_proc(void* params);
static int wd_wait(unsigned int timeout);

int wt_start_watchdog() {
    if(pthread_attr_init(&attr)) return 0;
    if(pthread_create(&id, &attr, &wd_proc, NULL)) return 0;
    ww_waitchdog_init();
    return 1;
}
void wt_stop_watchdog() {
    void *ret;

    wt_set_stop_watchdog();
    pthread_join(id, &ret);
    pthread_attr_destroy(&attr);
    ww_watchdog_destroy();
}
void wt_set_stop_watchdog() {
    stop = 1;
}

static void* wd_proc(void* params) {
    stop = 0;

    to_rp = wt_get_gueue(WT_WatchDogQueue);
    unsigned int to = wc_getWUDMonitoringTO();

    while(!stop && wd_wait(to)) {
        char* alert = ww_watchdog_analyze();
        if(alert) {                     //There is something to tell RP
            pu_queue_push(to_rp, alert, strlen(alert)+1);
            pu_log(LL_DEBUG, "%s: WD alert sent to main thread %s", PT_THREAD_NAME, alert);
            free(alert);
        }
    }
    pu_log(LL_INFO, "%s is finished", PT_THREAD_NAME);
    pthread_exit(NULL);
}
//Return 1 if TO, return 0 if stop on interrupt
static int wd_wait(unsigned int timeout) {
    unsigned int ret = sleep(timeout);
    if(!ret && !stop) return 1;  //got TO and no stop
    if(stop) return 0;
    return wd_wait(ret);
}