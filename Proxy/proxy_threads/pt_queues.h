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
    Proxy queues manager. Better to move it to the separate folder.
*/

#ifndef PRESTO_PT_QUEUES_H
#define PRESTO_PT_QUEUES_H

#include "pu_queue.h"

#define PS_MIN_QUEUE PS_FromAgentQueue      /* The event with min number. Needed because of default STOP event */
#define PS_MAX_QUEUE PS_FromReconnectQueue  /* The event with max number */

/* Full Proxy queues events list */
typedef enum {PS_Timeout = PQ_TIMEOUT,
    PS_FromAgentQueue = 1, PS_ToAgentQueue = 2, PS_FromServerQueue = 3, PS_ToServerQueue = 4, PS_ToWUDQueue = 5,
    PS_FromReconnectQueue = 6,
    PS_STOP = PQ_STOP} queue_events_t;

/**
 * Init Proxy queues service
*/
void init_queues();

/**
 * Stop Proxy queues service
*/
void erase_queues();

/**
 * Get the queue pointed by associated event number
 *
 * @param que_number    - one of queue_events_t numbers from PS_MIN_QUEUE up to PS_MAX_QUEUE
 *
 * @return  - pointer to the queue or NULL if no queue associated
 */
pu_queue_t* pt_get_gueue(int que_number);

#endif /* PRESTO_PT_QUEUES_H */
