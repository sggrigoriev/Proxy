//
// Created by gsg on 01/12/16.
//

#ifndef PRESTO_WM_MONITOR_H
#define PRESTO_WM_MONITOR_H

#include <stdlib.h>

int wm_init_monitor_data();
void wn_destroy_monitor_data();

//Return pointer to alert, NULL if nothing to tell
//NB! could issue alerts or commands for restart!
const char* wm_monitor();

#endif //PRESTO_WM_MONITOR_H
