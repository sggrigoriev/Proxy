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
    Created by gsg on 28/01/17.
*/

#ifndef PRESTO_PF_PROXY_COMMANDS_H
#define PRESTO_PF_PROXY_COMMANDS_H

#include <stdlib.h>

#include "cJSON.h"

#include "pu_queue.h"
#include "pr_commands.h"


/*********************************************************************
 * Make reconnect and send notification to the main proxy thread before  and after the reconnection
 * @param to_proxy_main     - point to input queue for the main proxy queue
 */
void pf_reconnect(pu_queue_t* to_proxy_main);


#endif /* PRESTO_PF_PROXY_COMMANDS_H */
