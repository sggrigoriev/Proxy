//
// Created by gsg on 29/10/16.
//
// Contains circular thread protected queue implementation. Multple reads/writes aloowed.
// Data element is char
//

#ifndef PRESTO_PU_QUEUE_H
#define PRESTO_PU_QUEUE_H

#include <pthread.h>

////////////////////////////////////////////////////
// Common part, concerning all queues
typedef enum {PS_Timeout=0, PS_FromServerQueue=1, PS_ToServerQueue=2, PS_ToAgentQueue=3, PS_FromAgentQueue=4, PS_Size} pu_queue_event_t;

typedef unsigned long pu_queue_events_set_t[PS_Size];


////////////////////////////////////////////////////
//Common data
pu_queue_events_set_t ps_all_queues_events_set;
pthread_mutex_t ps_all_queues_cond_mutex;            //Common mutex to protect condition set changes
pthread_cond_t ps_all_queues_cond;                   //condition event

///////////////////////////////////////////////////
// Common funtcions
int pu_queues_init();       // returns 0 if initiation fails
void pu_queues_destroy();
pu_queue_events_set_t* pu_add_queue_event(pu_queue_events_set_t* queue_events_mask, pu_queue_event_t event); //add event to the waiting list
pu_queue_events_set_t* pu_clear_queue_events(pu_queue_events_set_t* queue_events_mask); //remove all events from the waiting list
pu_queue_event_t pu_wait_for_queues(pu_queue_events_set_t* queue_events_mask, unsigned int to_sec); //wait for one or several queue events
///////////////////////////////////////////////////
// Single queue types & funcrions

typedef char pu_queue_msg_t;

typedef struct {
    pu_queue_msg_t* data;
    unsigned int len;   // if data is null-terminated string the len = strlen(data)+1;
} pu_queue_element_t;

typedef struct {
    pu_queue_element_t* q_array;
    unsigned long q_array_size;      // amount of q_array elements
    unsigned long rd_idx;            // first element to read
    unsigned long wr_idx;            // first empty element
    int overflow;                   // 1 if wr_idx gona replace unread data
    int empty;                      // 1 if no info in queue
    pu_queue_event_t event;         // the flag changed by the instance of queue (unique for the whole process!
    pthread_mutex_t own_mutex;     // syncing changes in queue

} pu_queue_t;

static const unsigned int DEFAULT_QUEUE_SIZE = 1024;

pu_queue_t* pu_queue_create(unsigned long records_amt, pu_queue_event_t my_event); //create the queue sertain size
void pu_queue_erase(pu_queue_t* queue);    //Dispose the queue

void pu_queue_push(pu_queue_t*, const pu_queue_msg_t* data, unsigned int len);    //Add data to queue
int pu_queue_pop(pu_queue_t*, pu_queue_msg_t* data, unsigned int* len); //Return 0 if no data. Get the data and len - queue element erased
int pu_queue_empty(pu_queue_t*);      //Non-blocking check. Returns 1 if the queue is empty

#endif //PRESTO_PU_QUEUE_H
