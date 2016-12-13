//
// Created by gsg on 12/12/16.
//

#ifndef PRESTO_PF_PROXY_WATCHDOG_H
#define PRESTO_PF_PROXY_WATCHDOG_H

void pf_wd_init();
//Return 1 if it is time to send watchdog alert to the WUD
int pf_wd_time_to_send();

#endif //PRESTO_PF_PROXY_WATCHDOG_H
