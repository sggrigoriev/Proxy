//
// Created by gsg on 12/12/16.
//

#ifndef LIB_TIMER_H
#define LIB_TIMER_H

#include <time.h>

typedef struct {
    time_t timeout;
    time_t last_update_time;
    time_t action_time;
}lib_timer_clock_t;

void lib_timer_init(lib_timer_clock_t* dat, time_t to);

void lib_timer_update(lib_timer_clock_t* dat);

int lib_timer_alarm(lib_timer_clock_t dat);

#endif //LIB_TIMER_H
