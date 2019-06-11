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
 Created by gsg on 19/05/19.
 Queue manager for test Agent
*/

#ifndef PRESTO_CC_QUEUES_H
#define PRESTO_CC_QUEUES_H

#include "pu_queue.h"

#define CC_MIN_QUEUE CC_FromReaderQueie  /* The event with min number. Needed because of default STOP event */
#define CC_MAX_QUEUE CC_ToWriterQueie  /* The event with max number */

/* Full Proxy queues events list */
typedef enum {CC_Timeout = PQ_TIMEOUT,
    CC_FromReaderQueie = 1, CC_ToWriterQueie = 2,
    CC_STOP = PQ_STOP} queue_events_t;

/**
 * Init comm_client queues service
*/
void init_queues();

/**
 * Stop comm_client queues service
*/
void erase_queues();

/**
 * Get the queue pointed by associated event number
 *
 * @param que_number    - from CC_MIN_QUEUE to CC_MAX_QUEUE
 * @return  - pointer to the queue or NULL if no queue associated
 */
pu_queue_t* cc_get_gueue(int que_number);

#endif /* PRESTO_CC_QUEUES_H */
