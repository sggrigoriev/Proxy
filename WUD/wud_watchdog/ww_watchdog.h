//
// Created by gsg on 01/12/16.
//

#ifndef PRESTO_WW_WATCHDOG_H
#define PRESTO_WW_WATCHDOG_H

//Return pointer to alert string or NULL

#include <pr_commands.h>

void ww_waitchdog_init();
void ww_watchdog_destroy();
void ww_watchdog_update(const char* who);

//Returns command for restart if timeout or null
//NB! User should memory returned!
char* ww_watchdog_analyze(); 

#endif //PRESTO_WW_WATCHDOG_H
