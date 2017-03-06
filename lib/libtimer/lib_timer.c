//
// Created by gsg on 12/12/16.
//
#include <sys/sysinfo.h>
#include <errno.h>
#include <string.h>

#include "pu_logger.h"
#include "lib_timer.h"


static time_t get_uptime();

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
    return dat.action_time < get_uptime();
}

static time_t get_uptime() {
    struct sysinfo info;

    if(sysinfo(&info) != 0) {
        pu_log(LL_ERROR, "get_uptime: sysinfo call does not work: %d, %s. System time is using instead", errno, strerror(errno));
        return time(NULL);
    }

    return (time_t)info.uptime;
}