//
// Created by gsg on 15/01/17.
//
#include <pthread.h>

#include "lib_timer.h"
#include "wm_childs_info.h"
#include "wa_alarm.h"

static pthread_mutex_t local_mutex = PTHREAD_MUTEX_INITIALIZER;

static lib_timer_clock_t ALARMS[PR_CHILD_SIZE] = {{0}};

void wa_alarms_init() {
    for(unsigned int i = 0; i < PR_CHILD_SIZE; i++) wa_alarm_reset((pr_child_t)i);
}

void wa_alarm_reset(pr_child_t proc) {
    if(!pr_child_t_range_check(proc)) return;
    pthread_mutex_lock(&local_mutex);
        lib_timer_init(&ALARMS[proc], wm_child_get_child_to(proc));
    pthread_mutex_unlock(&local_mutex);
}
void wa_alarm_update(pr_child_t proc) {
    if(!pr_child_t_range_check(proc)) return;
    pthread_mutex_lock(&local_mutex);
        lib_timer_update(&ALARMS[proc]);
    pthread_mutex_unlock(&local_mutex);
}

//Return 1 if timeout
int wa_alarm_wakeup(pr_child_t proc) {
    if(!pr_child_t_range_check(proc)) return 0;
    return lib_timer_alarm(ALARMS[proc]);
}
