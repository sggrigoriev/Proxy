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

    Contains circular thread protected queue implementation. Multiple async reads/writes alowed.
    Data element is char. 0-termination is not required
*/

#ifndef PRESTO_PU_QUEUE_H
#define PRESTO_PU_QUEUE_H

#include <pthread.h>
#include <stdint.h>

/* Special predefined event to pass timeout signal to threads connected to the queue */
#define PQ_TIMEOUT 0

/* TODO: Add pesistent layer to some queue(s) to be able to sent some alerts after resart */
/* TODO: Known problem: first wait_for_queue() should start earliar than pu_push() */
/*****************************************************************************************************
 Common part, concerning all queues
*/
typedef uint16_t pu_queue_event_t;

/* Special predefined event to pass STOP signal to threads connected to the queue */
#define PQ_STOP (sizeof(pu_queue_event_t)*8-1)
/*****************************************************************************************************
// Common funtctions
*/
/* Initiate queue service in a process
 *      queues_amount   - max queues amount
 *  Return 1 if OK, 0 if not
*/
int pu_queues_init(unsigned int queues_amount);

/* Destroy queues. Could be called at the very end of process
*/
void pu_queues_destroy();

/* Initiate events set associated with queues
 *  Return event set initiated. - Initially just PQ_TIMEOUT and PQ_STOP are defined
 */
pu_queue_event_t pu_create_event_set();

/* Add event to the waiting list to organize wait of multiple events
 *      queue_events_mask   - set of events
 *      event               - new event added
 *  Return the event set with new event added
 */
pu_queue_event_t pu_add_queue_event(pu_queue_event_t queue_events_mask, pu_queue_event_t event);

/* Wait for one or several queue events NB! PQ_STOP as well as PQ_TIMEOUT could come any time!
 *      queue_events_mask   - set of events could come
 *      to_sec              - timeout in seconds
 *  Return the evend happends. If the timeout exceeds - PQ_TIMEOUT came.
*/
pu_queue_event_t pu_wait_for_queues(pu_queue_event_t queue_events_mask, unsigned int to_sec);

/***********************************************************************************************
    Single queue types & funcrions
*/

/* Base data element in queue */
typedef char pu_queue_msg_t;

/* Queue element type */
typedef struct {
    pu_queue_msg_t* data;
    size_t len;   /* if data is null-terminated string the len = strlen(data)+1; */
} pu_queue_element_t;

/* Queue type */
typedef struct {
    pu_queue_element_t* q_array;
    size_t q_array_size;      /* amount of q_array elements */
    size_t rd_idx;            /* first element to read */
    size_t wr_idx;            /* first empty element */
    int overflow;                   /* 1 if wr_idx gona replace unread data */
    int empty;                      /* 1 if no info in queue */
    pu_queue_event_t event;         /* the flag changed by the instance of queue (unique for the whole process! */
    pthread_mutex_t own_mutex;      /* syncing changes in queue */
} pu_queue_t;

/* The default max queue elements allowed */
/* If this amount is achieved the oldest queue elements will be lost */
static const size_t DEFAULT_QUEUE_SIZE = 1024;

/* Create the queue sertain size
 *      records_amt     - max records amount in queue
 *      my_event        - the event associated with the queue. When the data arrives, the evend issued.
 *  Return the pointer to the queue
*/
pu_queue_t* pu_queue_create(unsigned long records_amt, pu_queue_event_t my_event);

/* Send the PQ_STOP to the queue. Just the way to stop thead(s) reading the queue
 *      queue   - pointer to the queue
*/
void pu_queue_stop(pu_queue_t* queue);

/* Dispose the queue
 *      queue   - pointer to the queue to be erased
 */
void pu_queue_erase(pu_queue_t* queue);

/* Add an element to queue
 *  data    - new element added
 *  len     - data length. (strlen(data)+1)
*/
void pu_queue_push(pu_queue_t*, const pu_queue_msg_t* data, size_t len);

/* Get the element from the queue. If the buffer is less than the element size the data will be truncated
 *      queue   - the data source queue
 *      data    - buffer for queue element returned
 *      len     - data buffer length NB! this is IO parameter: the actual data lenght will be returned
 *  Return 0 if no data, 1 if data copied
*/
int pu_queue_pop(pu_queue_t* queue, pu_queue_msg_t* data, size_t* len);

/* Non-blocking check for queue emptiness. For special funs of endless cycles in threads...
 *      queue   - the checking queue
 *  Return 1 if queue is empty, 0 if not
 */
int pu_queue_empty(pu_queue_t* queue);

#endif /*PRESTO_PU_QUEUE_H */
