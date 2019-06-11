/*
 *  Copyright 2018 People Power Company
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
 Created by gsg on 29/05/19.
 Short live thread to fulfill cloud command to change main URL
*/

#ifndef PRESTO_PT_RECONNECT_H
#define PRESTO_PT_RECONNECT_H

#include "cJSON.h"

/**
 * Start temp thread to proceed the main URL change
 *
 * @param main_url  - new main URL sent by cloud
 * @param cmd_id    - command id of corresponding cloud's command
 * @return  - 0 if error
 *          - 1 if Ok
 */
int start_reconnect(const char* main_url, unsigned long cmd_id);

/**
 * Kill reconnect thread. The thread will wait for 1 hour to be killed externally. Architecture...
 */
void kill_reconnect();

#endif /* PRESTO_PT_RECONNECT_H */
