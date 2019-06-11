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
    Created by gsg on 07/12/16.

    WUD queue manager. Should be shifted from wid_threads directory
*/

#ifndef PRESTO_WT_QUEUES_H
#define PRESTO_WT_QUEUES_H

#include "pu_queue.h"

#define WT_MIN_QUEUE WT_to_Main
#define WT_MAX_QUEUE WT_to_Cloud

/* WUD queue events. TIMEOUT & STOP are predefined */
typedef enum {WT_Timeout = PQ_TIMEOUT,
    WT_to_Main = 1, WT_to_Cloud = 2,
    WT_STOP = PQ_STOP} wt_queue_events_t;

/**
 * Initiate WUD queue manager
 */
void wt_init_queues();

/**
 * Stop WUD queue manager. It does not called because WUD is endless :-)
 */
void wt_erase_queues();

/**
 * Get the pointer to the queue by associated event number
 *
 * @param que_number    - queue (event) number from WT_MIN_QUEUE to WT_MAX_QUEUE
 * @return              - queue pointer or NULL if no such an association
 */
pu_queue_t* wt_get_gueue(int que_number);


#endif /* PRESTO_WT_QUEUES_H */
