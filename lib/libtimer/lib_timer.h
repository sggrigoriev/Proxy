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
//
// Created by gsg on 12/12/16.
//
//  Gateway time service for timeouts and periodical actions start. Works with server uptime as base because of funny time jumps during the GW initiation
//
#ifndef LIB_TIMER_H
#define LIB_TIMER_H

#include <time.h>

//Timer datatype
typedef struct {
    time_t timeout;
    time_t last_update_time;
    time_t action_time;
}lib_timer_clock_t;

//Initiates the timer
//  dat - timer data to be initiated
//  to  - alarm delta (timeout) in seconds
void lib_timer_init(lib_timer_clock_t* dat, time_t to);

//Reset the timer for the timeout set on init step
//  dat - timer to be updated
void lib_timer_update(lib_timer_clock_t* dat);

//Check for alarm
//  dat - timer to be analyzed
//Return 1 if alarm; 0 if not
int lib_timer_alarm(lib_timer_clock_t dat);

#endif //LIB_TIMER_H
