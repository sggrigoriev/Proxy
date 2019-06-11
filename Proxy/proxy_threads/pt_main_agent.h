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
    Created by gsg on 06/12/16.
    The manager read/write threads: wait for Agent start, reconnect read/write threads in case of Agent fails
*/

#ifndef PRESTO_PT_MAIN_AGENT_H
#define PRESTO_PT_MAIN_AGENT_H

/**
 * Start the thread.
 *
 * @return  - 1
*/
int start_agent_main();

/**
 * Stop the thread
*/
void stop_agent_main();

/**
 * Set stop to the thread - external interface for async thread stop
 */
void set_stop_agent_main();

/**
 * Set stop to the r/w threads - external interface for async threads stop
*/
void set_stop_agent_children();

/**
 * Check the stop flag. Return 1 if flag is on, 0 if not
 *
 * @return  - 0 if flag is off
 *          - 1 if flag is on
*/
int is_childs_stop();

#endif /* PRESTO_PT_MAIN_AGENT_H */
