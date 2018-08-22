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
 *
//
// Created by gsg on 12/12/16.
*/
#include <sys/sysinfo.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>

#include "pu_logger.h"
#include "lib_timer.h"

/*Get the gateway uptime from sysinfo or system time if again, the GW OS has some POSIX incompatibilities :-(
//Return abs Unix time in seconds
*/
static time_t get_uptime() {
    struct sysinfo info;

    if(sysinfo(&info) != 0) {
        pu_log(LL_ERROR, "get_uptime: sysinfo call does not work: %d, %s. System time is using instead", errno, strerror(errno));
        return time(NULL);
    }

    return (time_t)info.uptime;
}

/*//////////////////////////////////////////////////////////////////
//Public funcions (description in *.h)
*/
void lib_timer_init(lib_timer_clock_t* dat, time_t to) {
    dat->timeout = to;
    dat->last_update_time = get_uptime();
    dat->action_time = dat->last_update_time + to;
}

void lib_timer_update(lib_timer_clock_t* dat) {
    dat->last_update_time = get_uptime();
    dat->action_time = dat->last_update_time + dat->timeout;
}

int lib_timer_alarm(lib_timer_clock_t dat) {
    time_t now = get_uptime();
    if (dat.action_time < now) {
        pu_log(LL_DEBUG, "%s ALARM!!! Delta = %d", __FUNCTION__, dat.action_time - now);
    }

    return dat.action_time < now;
}

void lib_timer_sleep(int to_counter, int max_to_amount, long u_to, unsigned int s_to) {
    if(to_counter < max_to_amount) {
        struct timespec t = {0, u_to},rem;
        nanosleep(&t, &rem);
    }
    else {
        sleep(s_to);
    }
}

