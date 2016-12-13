//
// Created by gsg on 12/12/16.
//

#ifndef PRESTO_PF_PROXY_WATCHDOG_H
#define PRESTO_PF_PROXY_WATCHDOG_H

#include <time.h>

typedef struct {
    time_t last_update_time;
    time_t wd_sent_time;
}pf_clock_t;

//Return 1 if it is time to send watchdog alert to the WUD
int pf_wd_time_to_send(pf_clock_t dat);

#endif //PRESTO_PF_PROXY_WATCHDOG_H
