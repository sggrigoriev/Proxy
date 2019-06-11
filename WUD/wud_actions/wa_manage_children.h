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
    Created by gsg on 30/11/16.

    Currently uses just for Proxy start. The Agent start is disabled.
    To use it each child process should create the file with own pid number...
    Used in full strength with Agent emulator
*/

#ifndef PRESTO_WA_MANAGE_CHILDREN_H
#define PRESTO_WA_MANAGE_CHILDREN_H


#include <pr_commands.h>

/**
 * Start child process
 *
 * @param id    - child descriptor id
 * @return  - 1 if ok, 0 if not
 */
int wa_start_child(pr_child_t id);

/**
 * Restart child process
 *
 * @param id  - child descriptor id
 * @return  - 1 if ok, 0 if not
 */
int wa_restart_child(pr_child_t id);

/**
 * Stop child process
 *
 * @param id    - child descriptor id
 */
void wa_stop_child(pr_child_t id);

/**
 * Stop all child processes registered
*/
void wa_stop_children();

#endif /* PRESTO_WA_MANAGE_CHILDREN_H */
