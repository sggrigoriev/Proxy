//
// Created by gsg on 08/12/16.
//
#include <pthread.h>
#include <string.h>
#include <stdlib.h>

#include "wa_alarm.h"
#include "pu_logger.h"
#include "wc_settings.h"
#include "wt_queues.h"

#include "wt_watchdog.h"

#define PT_THREAD_NAME "WATCHDOG"

////////////////////////////
static pthread_t id;
static pthread_attr_t attr;
static volatile int stop;

static pu_queue_t* to_main;

static void* wd_proc(void* params);
static int wd_wait(unsigned int timeout);

int wt_start_watchdog() {
    if(pthread_attr_init(&attr)) return 0;
    if(pthread_create(&id, &attr, &wd_proc, NULL)) return 0;
    wa_alarms_init();
    return 1;
}
void wt_stop_watchdog() {
    void *ret;

    wt_set_stop_watchdog();
    pthread_join(id, &ret);
    pthread_attr_destroy(&attr);
}
void wt_set_stop_watchdog() {
    stop = 1;
}

static void* wd_proc(void* params) {
    stop = 0;

    to_main = wt_get_gueue(WT_to_Main);
    unsigned int to = wc_getWUDMonitoringTO();

    while(!stop && wd_wait(to)) {
        unsigned int i;
        for(i = 0; i < PR_CHILD_SIZE; i++) {
            if (wa_alarm_wakeup((pr_child_t) i)) {
                char restart[LIB_HTTP_MAX_MSG_SIZE];

                pr_make_cmd_restart(pr_chld_2_string((pr_child_t) i), restart, sizeof(restart));
                pu_queue_push(to_main, restart, strlen(restart) + 1);
                pu_log(LL_DEBUG, "%s: WD alert sent to main thread %s", PT_THREAD_NAME, restart);
            }
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