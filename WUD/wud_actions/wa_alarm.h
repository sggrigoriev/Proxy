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
    Created by gsg on 15/01/17.

    WUD timer service oriented to registered child processes. Now it is Proxy & Agent
*/

#ifndef PRESTO_WA_ALARM_H
#define PRESTO_WA_ALARM_H

#include "pr_commands.h"

/* Initiait alarm service */
void wa_alarms_init();

/* Reset child's alarm - the imeout set in child's descriptor (see manage_children)*/
void wa_alarm_reset(pr_child_t proc);

/* Update child's alarm - add same timeout as before*/
void wa_alarm_update(pr_child_t proc);

/* Return 1 if timeout, o if not */
int wa_alarm_wakeup(pr_child_t proc);

#endif /* PRESTO_WA_ALARM_H */
