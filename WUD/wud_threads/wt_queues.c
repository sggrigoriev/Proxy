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
*/

#include "wc_settings.h"
#include "wt_queues.h"

/***************************************************************
 * Local data
 */
static pu_queue_t* qu_arr[WT_MAX_QUEUE-WT_MIN_QUEUE+1]; /* WUD queues pool */
static pthread_mutex_t  own_mutex;  /* Queues pool protection */

/****************************************************************
 * Public finctions implementation
 */
void wt_init_queues() {
    pthread_mutex_lock(&own_mutex);

    pu_queues_init(WT_MAX_QUEUE-WT_MIN_QUEUE+1);
    pu_queue_event_t i;
    for(i = 0; i < WT_MAX_QUEUE-WT_MIN_QUEUE+1; i++) {
        qu_arr[i] = pu_queue_create(wc_getQueuesRecAmt(),i+WT_MIN_QUEUE);
        pu_log(LL_DEBUG, "wt_init_queues: queue# %d - %lu", i, qu_arr[i]);
    }
    pthread_mutex_unlock(&own_mutex);
}

void wt_erase_queues() {
    pthread_mutex_lock(&own_mutex);
    unsigned int i;
    for(i = 0; i < WT_MAX_QUEUE-WT_MIN_QUEUE+1; i++) {
        pu_queue_erase(qu_arr[i+WT_MIN_QUEUE]);
    }
    pu_queues_destroy();
    pthread_mutex_unlock(&own_mutex);
}

pu_queue_t* wt_get_gueue(int que_number) {
    if(que_number < WT_MIN_QUEUE) return NULL;
    if(que_number >WT_MAX_QUEUE) return NULL;

    pthread_mutex_lock(&own_mutex);
    pu_queue_t* ret = qu_arr[que_number-WT_MIN_QUEUE];
    pthread_mutex_unlock(&own_mutex);

    return ret;
}