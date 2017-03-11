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

    Proxy queue manager. Better to move it to the separate folder...
*/

#ifndef PRESTO_PT_QUEUES_H
#define PRESTO_PT_QUEUES_H

#include "pu_queue.h"

#define PS_MIN_QUEUE PS_FromAgentQueue  /* The event with min number. Needed because of default STOP event */

#ifndef PROXY_SEPARATE_RUN              /* define set in Proxy CMakeLists. Needed to run Proxy w/o WUD in debugging purposes */
    #define PS_MAX_QUEUE PS_ToWUDQueue  /* The event with max number */
#else
    #define PS_MAX_QUEUE PS_ToServerQueue
#endif

/* Full Proxy queues events list */
typedef enum {PS_Timeout = PQ_TIMEOUT,
    PS_FromAgentQueue = 1, PS_ToAgentQueue = 2, PS_FromServerQueue = 3, PS_ToServerQueue = 4,
#ifndef PROXY_SEPARATE_RUN
    PS_ToWUDQueue = 5,
#endif
    PS_STOP = PQ_STOP} queue_events_t;

/* Init Proxy queues service */
void init_queues();

/* Stop Proxy queues service */
void erase_queues();

/* Get the queue pointed by associated eveny number
 *  Return pointer to the queue or NULL if no queue associated
 */
pu_queue_t* pt_get_gueue(int que_number);

#endif /* PRESTO_PT_QUEUES_H */
