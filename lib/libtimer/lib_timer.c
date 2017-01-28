//
// Created by gsg on 12/12/16.
//

#include "lib_timer.h"

void lib_timer_init(lib_timer_clock_t* dat, time_t to) {
    dat->timeout = to;
    dat->last_update_time = time(NULL);
    dat->action_time = dat->last_update_time + to;
}

void lib_timer_update(lib_timer_clock_t* dat) {
    dat->last_update_time = time(NULL);
    dat->action_time = dat->last_update_time + dat->timeout;
}

int lib_timer_alarm(lib_timer_clock_t dat) {
    return dat.action_time < time(NULL);
}