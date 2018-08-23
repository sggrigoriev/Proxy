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
    Created by gsg on 15/01/17.
*/
#include <pthread.h>

#include "lib_timer.h"

#include "wm_childs_info.h"

#include "wa_alarm.h"

static pthread_mutex_t local_mutex = PTHREAD_MUTEX_INITIALIZER; /* Concurrent use ALARMS protection */

static volatile lib_timer_clock_t ALARMS[PR_CHILD_SIZE] = {{0}};         /* Child's timers */

/*****************************************************************************************
 * Public functions implementaiton
 */

void wa_alarms_init() {
    unsigned int i;
    for(i = 0; i < PR_CHILD_SIZE; i++) wa_alarm_reset((pr_child_t)i);
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

int wa_alarm_wakeup(pr_child_t proc) {
    if(!pr_child_t_range_check(proc)) return 0;
    pthread_mutex_lock(&local_mutex);
        int is_alarm = lib_timer_alarm(ALARMS[proc]);
    pthread_mutex_unlock(&local_mutex);
    return is_alarm;
}
