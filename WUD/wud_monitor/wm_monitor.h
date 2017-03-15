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
/*
    Created by gsg on 01/12/16.

    Gateway system resources monitoring service. Not implemented.
*/

#ifndef PRESTO_WM_MONITOR_H
#define PRESTO_WM_MONITOR_H

#include <stdlib.h>

/**************************************************************
 * Initiate monitoring service
 * @return  - 1 if OK, 0 if not
 */
int wm_init_monitor_data();

/**************************************************************
 * Close monitoring service
 */
void wn_destroy_monitor_data();

//Return pointer to alert, NULL if nothing to tell
//NB! could issue alerts or commands for restart!
/**************************************************************
 * Check system resources state
 * @return  - pointer to alert message if recource(s) low, or NULL if OK
 */
const char* wm_monitor();

#endif /* PRESTO_WM_MONITOR_H */
