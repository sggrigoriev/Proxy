//
// Created by gsg on 29/10/16.
//

#include <stdlib.h>
#include <sys/time.h>
#include <errno.h>
#include <string.h>

#include "pu_logger.h"
#include "pu_queue.h"

////////////////////////////////////////////////////
//Common local data
static pu_queue_event_t* ps_all_queues_events_set = NULL;
static pthread_mutex_t ps_all_queues_cond_mutex;            //Common mutex to protect condition set changes
static pthread_cond_t ps_all_queues_cond;                   //condition event
static unsigned int PQ_Size;
/////////////////////////////////////////////////////////////
//
static unsigned long inc_idx(const pu_queue_t* q, unsigned long idx) {
    return (idx == q->q_array_size-1)?0 : idx+1;
}
static void erase_element(pu_queue_t* q, unsigned long idx) {
    if(q->q_array[idx].data) (free(q->q_array[idx].data), q->q_array[idx].data = NULL);
    q->q_array[idx].len = 0;
}
static int check_ptr(const void* ptr, const char* phrase) {
    if(!ptr) {
        pu_log(LL_ERROR, "%s", phrase);
        return 0;
    }
    return 1;
}
static int check_params_for_pu_queue_push(const pu_queue_t* queue, const pu_queue_msg_t* data, unsigned long len) {
    if (!len) {
        pu_log(LL_WARNING, "Queue %d received zero length message. Ignored", queue->event);
        return 0;
    }
    if(!check_ptr(queue, "pu_queue_push() got NULL \'queue\' parameter. Failed.")) return 0;
    if(!check_ptr(data, "pu_queue_push() got NULL \'data\' parameter. Failed")) return 0;
    return 1;
}
///////////////////////////////////////////////////
// Common funtcions
//
//pu_queues_init - NB! it must be called once in initiation section before using any queue in a process!!
//
int pu_queues_init(unsigned int queues_amount) {       // returns 0 if initiation fails
    pthread_mutex_init(&ps_all_queues_cond_mutex, NULL);
    pthread_cond_init (&ps_all_queues_cond, NULL);
    if(ps_all_queues_events_set) {
        pu_log(LL_ERROR, "pu_queues_init: Queues already initiated. Initiation ignored");
        return 0;
    }
    PQ_Size = queues_amount + 1; //Adding default event "PQ_TIMEOUT"
    ps_all_queues_events_set = malloc(PQ_Size* sizeof(pu_queue_event_t));
    for(unsigned int i = 0; i < PQ_Size; i++) ps_all_queues_events_set[i] = 0;
    return 1;
}
//
//pu_queues_destroy - NB! it must be called once in termintation part of a process after all queues are destroyed!
//
void pu_queues_destroy() {
    pthread_mutex_destroy(&ps_all_queues_cond_mutex);
    pthread_cond_destroy(&ps_all_queues_cond);
}
pu_queue_event_t* pu_create_event_set() {
    pu_queue_event_t* ret = (pu_queue_event_t* )malloc(PQ_Size*sizeof(pu_queue_event_t));
    if(ret) memset(ret, (pu_queue_event_t)0, PQ_Size);
    return ret;
}
void pu_delete_event_set(pu_queue_event_t* es) {
    if(es) free(es);
}
pu_queue_event_t* pu_add_queue_event(pu_queue_event_t* queue_events_mask, pu_queue_event_t event) { //add event to the waiting list
    if(!check_ptr(queue_events_mask, "pu_add_queue_event() got NULL \'queue_events_mask\' parameter. Failed.")) return NULL;
    if(event >= PQ_Size) {
        pu_log(LL_ERROR, "pu_add_queue_event: event# exceeds event set size. Failed.");
        return NULL;
    }
    queue_events_mask[event] = 1;
    return queue_events_mask;
}
pu_queue_event_t* pu_clear_queue_events(pu_queue_event_t* queue_events_mask) { //remove all events from the waiting list
    if(!check_ptr(queue_events_mask, "pu_clear_queue_event() got NULL \'queue_events_mask\' parameter. Failed.")) return NULL;
    for(pu_queue_event_t i = 0; i < PQ_Size; i++)
    queue_events_mask[i] = 0;
    return queue_events_mask;
}
//
//pu_wait_for_queues - returns the first queue in the queue_events_mask which has data or wait
//
//queue_events_set - waiting list for data in queues
//to_sec - timeout in seconds. if to_sec == 0 - wait forever
//returns the first queue from queue_events_set with data occures
//
pu_queue_event_t pu_wait_for_queues(pu_queue_event_t* queue_events_set, unsigned int to_sec) { //wait for one or several queue events
    pu_queue_event_t ret;

    struct timespec timeToWait;
    struct timeval now;
    int rt;

    gettimeofday(&now, NULL);

    timeToWait.tv_sec = now.tv_sec+to_sec;
    timeToWait.tv_nsec = 0;

    pthread_mutex_lock(&ps_all_queues_cond_mutex);
    wait:
    if(to_sec) {
        rt = pthread_cond_timedwait(&ps_all_queues_cond, &ps_all_queues_cond_mutex, &timeToWait);
        if (rt == ETIMEDOUT) {
            pthread_mutex_unlock(&ps_all_queues_cond_mutex);
            return PQ_TIMEOUT;
        }
    }
    else {
        pthread_cond_wait(&ps_all_queues_cond, &ps_all_queues_cond_mutex);
    }

    ret = PQ_TIMEOUT;
    for(pu_queue_event_t i = PQ_TIMEOUT+1; i < PQ_Size; i++) {
        if((queue_events_set[i]) && (ps_all_queues_events_set[i] > 0)) {
            ps_all_queues_events_set[i] = 0;
            ret = i;
        }
    }
    if(ret == PQ_TIMEOUT) goto wait;    //That was not our condition!
    pthread_mutex_unlock(&ps_all_queues_cond_mutex);
    return ret;
}
/////////////////////////////////////////////////////////////
//

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
    for(unsigned long i = 0; i < queue->q_array_size; i++) {
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

    for(unsigned long i = 0; i < queue->q_array_size; i++) erase_element(queue, i);
}
void pu_queue_push(pu_queue_t* queue, const pu_queue_msg_t* data, size_t len) {

    if(!check_params_for_pu_queue_push(queue, data, len)) return;

    pthread_mutex_lock(&queue->own_mutex);

//check for possible overflow
    if((queue->wr_idx == queue->rd_idx) && !queue->empty) { // overflow case!
        queue->overflow = 1;
        pu_log(LL_WARNING, "Queue %d overflow!. Lost msg: %s", queue->event, queue->q_array[queue->rd_idx].data);
        erase_element(queue, queue->rd_idx);
        queue->rd_idx = inc_idx(queue, queue->rd_idx);
    }
//add new element to the queue
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

//send the signal we got smth
    pthread_mutex_lock(&ps_all_queues_cond_mutex);
       ps_all_queues_events_set[queue->event]++;
       pthread_cond_broadcast(&ps_all_queues_cond);           //Who if first - owns the slippers
    pthread_mutex_unlock(&ps_all_queues_cond_mutex);

    pthread_mutex_unlock(&queue->own_mutex);
}
//returns 0 if no data
int pu_queue_pop(pu_queue_t* queue, pu_queue_msg_t* data, size_t* len) {
    if(!check_ptr(queue, "pu_queue_pop() got NULL \'queue\' parameter. Failed.")) return 0;
    if(!check_ptr(data, "pu_queue_pop() got NULL \'data\' parameter. Failed.")) return 0;
    if(!check_ptr(len, "pu_queue_pop() got NULL \'len\' parameter. Failed.")) return 0;

    pthread_mutex_lock(&queue->own_mutex);
    if(queue->empty) {
        pthread_mutex_unlock(&queue->own_mutex);
        return 0;
    }

    *len = (queue->q_array[queue->rd_idx].len > *len)? *len : queue->q_array[queue->rd_idx].len;
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

