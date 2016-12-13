//
// Created by gsg on 12/12/16.
//

#include <pc_settings.h>
#include "pf_proxy_watchdog.h"


//Return 1 if it is time to send watchdog alert to the WUD
int pf_wd_time_to_send(pf_clock_t dat) {
    dat.last_update_time = time(NULL);
    if((dat.wd_sent_time + pc_getAgentWDTO()) <= dat.last_update_time) {
        dat.wd_sent_time = dat.last_update_time;
        return 1;
    }
    return 0;
}
