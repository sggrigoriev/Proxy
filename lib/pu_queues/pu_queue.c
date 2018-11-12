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
    Created by gsg on 29/10/16.
*/

#if __STDC_VERSION__ >= 199901L
#define _XOPEN_SOURCE 600
#else
#define _XOPEN_SOURCE 500
#endif /* __STDC_VERSION__ */

#include <time.h>
#include <stdlib.h>
#include <sys/time.h>
#include <errno.h>
#include <string.h>

#include "pu_logger.h"
#include "pu_queue.h"

extern uint32_t contextId;

/*****************************************************************************************************
    Common local data
*/
/* common events set for all queues in a process */
static pu_queue_event_t ps_all_queues_events_set = 0;

/* Common mutex to protect condition set changes */
static pthread_mutex_t ps_all_queues_cond_mutex;

/* condition event */
static pthread_cond_t ps_all_queues_cond;

/* Max size of totally possible events declared. Currently - 1 byte. To have more - change the pu_queue_event_t base type from char to smth bigger */
static unsigned int PQ_Size = sizeof(pu_queue_event_t)*8;

/********************************************************************************************************
    Local functions
*/
/* Round Robin cyclic index imlementation (the second one in Proxy)
 *      q   - the queue
 *      idx - current index (pointed to the first free or to the last occupied place - depends on use
 *  Return index incremented
 */
static unsigned long inc_idx(const pu_queue_t* q, unsigned long idx) {
    return (idx == q->q_array_size-1)?0 : idx+1;
}

/*  Erase queue element
 *      q   - the queue
 *      idx - the destroyed element's index
*/
static void erase_element(pu_queue_t* q, unsigned long idx) {
    if(q->q_array[idx].data) (free(q->q_array[idx].data), q->q_array[idx].data = NULL);
    q->q_array[idx].len = 0;
}

/* Create the mask for the evend
 *      event_number    - event
 *  Return the event mask - all 1s and zero bit at the event's number place
*/
static pu_queue_event_t make_event_mask(pu_queue_event_t event_number) {
    const pu_queue_event_t mask = 1;
    return mask << (PQ_Size - 1 - event_number);
}

/* Checks the ptr validity, write to the log some words if ptr bad
 *      ptr     - poiter examined
 *      phrase  - char string with all words about the disaster
 *  Return 1 if ptr is not NULL, 0 - if ptr is NULL
 */
static int check_ptr(const void* ptr, const char* phrase) {
    if(!ptr) {
        pu_log(LL_ERROR, "%s", phrase);
        return 0;
    }
    return 1;
}

/* Check validity for puch function parameters.
 *      queue   - pushed queue
 *      data    - pushed data
 *      len     - data length in bytes
 *  Return 1 if params are OK, 0 if not
 */
static int check_params_for_pu_queue_push(const pu_queue_t* queue, const pu_queue_msg_t* data, unsigned long len) {
    if (!len) {
        pu_log(LL_WARNING, "Queue %d received zero length message. Ignored", queue->event);
        return 0;
    }
    if(!check_ptr(queue, "pu_queue_push() got NULL \'queue\' parameter. Failed.")) return 0;
    if(!check_ptr(data, "pu_queue_push() got NULL \'data\' parameter. Failed")) return 0;
    return 1;
}

/******************************************************************************************************************
    Public funtcions
*/

int pu_queues_init(unsigned int queues_amount) {       /* returns 0 if initiation fails */
    if(queues_amount > PQ_Size-2) {     /*zero bit is reserved for timeout fake event; the last bit reserved for STOP event */
        pu_log(LL_ERROR, "pu_queues_init: Queues amount requested greater than max possible amount: %d > %d. Initiation ignored", queues_amount, PQ_Size-2);
        return 0;
    }
    pthread_mutex_init(&ps_all_queues_cond_mutex, NULL);
    pthread_cond_init (&ps_all_queues_cond, NULL);
    ps_all_queues_events_set = 0;
    return 1;
}

void pu_queues_destroy() {
    pthread_mutex_destroy(&ps_all_queues_cond_mutex);
    pthread_cond_destroy(&ps_all_queues_cond);
}

pu_queue_event_t pu_create_event_set() {
    return make_event_mask(PQ_STOP);
}

pu_queue_event_t pu_add_queue_event(pu_queue_event_t queue_events_mask, pu_queue_event_t event) { /* add event to the waiting list */
    if((event >= PQ_Size-1)||(!event)) {
        pu_log(LL_ERROR, "pu_add_queue_event: event# %d not fit into event set size %d. Event is not added.", event, PQ_Size-1);
        return queue_events_mask;
    }
    return (queue_events_mask | make_event_mask(event));
}
/*
 * Modification:
 * Even if TO happens - check the events set: anyway. If nothing - goto timeout
 * NB! If we got event from other thread - our thread will have TO.
 */
pu_queue_event_t pu_wait_for_queues(pu_queue_event_t queue_events_set, unsigned int to_sec) { /* wait for one or several queue events */
    pu_queue_event_t ret;
    struct timespec timeToWait;
    struct timeval now;

    contextId = 1;

    gettimeofday(&now, NULL);

    contextId = 2;

    timeToWait.tv_sec = now.tv_sec+to_sec;
    contextId = 3;
    timeToWait.tv_nsec = 0;
    contextId = 4;
    pthread_mutex_lock(&ps_all_queues_cond_mutex);
    contextId = 5;
    if(to_sec) {
        contextId = 6;
        pthread_cond_timedwait(&ps_all_queues_cond, &ps_all_queues_cond_mutex, &timeToWait);
        contextId = 7;
    }
    else {
        contextId = 8;
        pthread_cond_wait(&ps_all_queues_cond, &ps_all_queues_cond_mutex);
        contextId = 9;
    }
    contextId = 10;
    ret = PQ_TIMEOUT;
    pu_queue_event_t i;
    contextId = 11;
    for(i = 1; i < PQ_Size; i++) {
        contextId = 12;
        if(((queue_events_set >> (PQ_Size - 1 - i))&(pu_queue_event_t)1) && ((ps_all_queues_events_set >> (PQ_Size - 1 - i))&(pu_queue_event_t)1)) {
            contextId = 13;
            ps_all_queues_events_set &= ~make_event_mask(i);
            contextId = 14;
            ret = i;
            contextId = 15;
        }
        contextId = 16;
    }
    contextId = 17;
    pthread_mutex_unlock(&ps_all_queues_cond_mutex);
    contextId = 18;
    return ret;
}

pu_queue_t* pu_queue_create(unsigned long records_amt, pu_queue_event_t my_event) {

    pu_queue_t* queue;
    if(queue = malloc(sizeof(pu_queue_t)), queue == NULL) {
        pu_log(LL_ERROR, "Queue %d creation: not enough memory to create the queue. Failed.", my_event);
        return NULL;
    }

    queue->q_array_size = (records_amt)?records_amt:DEFAULT_QUEUE_SIZE;
    if(queue->q_array = malloc(queue->q_array_size * sizeof(pu_queue_element_t)), queue->q_array == NULL) {
        free(queue);
        pu_log(LL_ERROR, "Queue %d creation: not enough memory to create the queue array. Failed.", my_event);
        return NULL;
    }
    unsigned long i;
    for(i = 0; i < queue->q_array_size; i++) {
        queue->q_array[i].data = NULL;
        queue->q_array[i].len = 0;
    }

    queue->rd_idx = 0;
    queue->wr_idx = 0;
    queue->empty = 1;
    queue->overflow = 0;
    queue->event = my_event;
    if(pthread_mutex_init(&queue->own_mutex, NULL) != 0) {
        pu_log(LL_ERROR, "pthread_mutex_init error: %d, %s", errno, strerror(errno));
        free(queue->q_array);
        free(queue);
        return NULL;
    }

    return queue;
}

void pu_queue_erase(pu_queue_t* queue) {

    if(!check_ptr(queue, "pu_queue_erase() got NULL \'queue\' parameter. Failed.")) return;
    unsigned long i;
    for(i = 0; i < queue->q_array_size; i++) erase_element(queue, i);
}

void pu_queue_stop(pu_queue_t* queue) {
    pthread_mutex_lock(&ps_all_queues_cond_mutex);

    ps_all_queues_events_set |= make_event_mask(PQ_STOP);
    pthread_cond_broadcast(&ps_all_queues_cond);           /* Who is the first - owns the slippers! */

    pthread_mutex_unlock(&ps_all_queues_cond_mutex);
}

void pu_queue_push(pu_queue_t* queue, const pu_queue_msg_t* data, size_t len) {

    if(!check_params_for_pu_queue_push(queue, data, len)) return;
    pthread_mutex_lock(&queue->own_mutex);

/* check for possible overflow */
    if((queue->wr_idx == queue->rd_idx) && !queue->empty) { /* overflow case! */
        queue->overflow = 1;
        pu_log(LL_WARNING, "Queue %d overflow!. Lost msg: %s", queue->event, queue->q_array[queue->rd_idx].data);
        erase_element(queue, queue->rd_idx);
        queue->rd_idx = inc_idx(queue, queue->rd_idx);
    }
/* add new element to the queue */
    if(queue->q_array[queue->wr_idx].data = malloc(len * sizeof(pu_queue_msg_t)), !queue->q_array[queue->wr_idx].data) {
        pu_log(LL_ERROR, "Queue %d: not enough memory to add data!", queue->event);
        pu_log(LL_WARNING, "Queue %d lost data: %s", queue->event, data);
        pthread_mutex_unlock(&queue->own_mutex);
        return;
    }
    queue->q_array[queue->wr_idx].len = len;
    memcpy(queue->q_array[queue->wr_idx].data, data, len*sizeof(pu_queue_msg_t));
    queue->wr_idx = inc_idx(queue, queue->wr_idx);
    queue->empty = 0;

/* send the signal we got smth */
    pthread_mutex_lock(&ps_all_queues_cond_mutex);
       ps_all_queues_events_set |= make_event_mask(queue->event);
       pthread_cond_broadcast(&ps_all_queues_cond);           /* Who is the first - owns the slippers! */
    pthread_mutex_unlock(&ps_all_queues_cond_mutex);

    pthread_mutex_unlock(&queue->own_mutex);
}

int pu_queue_pop(pu_queue_t* queue, pu_queue_msg_t* data, size_t* len) {
    if(!check_ptr(queue, "pu_queue_pop() got NULL \'queue\' parameter. Failed.")) return 0;
    if(!check_ptr(data, "pu_queue_pop() got NULL \'data\' parameter. Failed.")) return 0;
    if(!check_ptr(len, "pu_queue_pop() got NULL \'len\' parameter. Failed.")) return 0;

    pthread_mutex_lock(&queue->own_mutex);
    if(queue->empty) {
        pthread_mutex_unlock(&queue->own_mutex);
        return 0;
    }

    if(*len < queue->q_array[queue->rd_idx].len) {
        pu_log(LL_WARNING, "pu_queue_pop: too small buffer: data truncated");
    }
    else {
        *len = queue->q_array[queue->rd_idx].len;
    }
    memcpy(data, queue->q_array[queue->rd_idx].data, *len);
    free(queue->q_array[queue->rd_idx].data);
    queue->q_array[queue->rd_idx].data = NULL;
    queue->q_array[queue->rd_idx].len = 0;
    queue->rd_idx = inc_idx(queue, queue->rd_idx);
    queue->overflow = 0;
    queue->empty = (queue->rd_idx == queue->wr_idx);

    pthread_mutex_unlock(&queue->own_mutex);
    return 1;
}

int pu_queue_empty(pu_queue_t* queue) {
    int ret;
    if(!check_ptr(queue, "pu_queue_empty() got NULL \'queue\' parameter. Failed.")) return 0;
    pthread_mutex_lock(&queue->own_mutex);
    ret = queue->empty ;
    pthread_mutex_unlock(&queue->own_mutex);
    return ret;
}

