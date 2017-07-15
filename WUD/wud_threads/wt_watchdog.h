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
    Created by gsg on 08/12/16.
    WUD watchdog  thread. Send alarm to request processor if one of children didn't send thr watchdog message on time
*/

#ifndef WDU_WT_WATCHDOG_H
#define WDU_WT_WATCHDOG_H

/****************************
 * Start thread
 * @return  - 1if OK, 0 if not
 */
int wt_start_watchdog();

/****************************
 * Stop thread
 */
void wt_stop_watchdog();

/***************************
 * Set the flag for async thread stop
 */
void wt_set_stop_watchdog();

#endif /* WDU_WT_WATCHDOG_H */
