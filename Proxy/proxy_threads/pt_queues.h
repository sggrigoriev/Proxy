//
// Created by gsg on 06/12/16.
//

#ifndef PRESTO_PT_QUEUES_H
#define PRESTO_PT_QUEUES_H

#include "pu_queue.h"

#define PS_MIN_QUEUE PS_FromAgentQueue
#define PS_MAX_QUEUE PS_ToServerQueue

typedef enum {PS_Timeout = PQ_TIMEOUT,
    PS_FromAgentQueue = 1, PS_ToAgentQueue = 2, PS_FromServerQueue = 3, PS_ToServerQueue = 4,
#ifndef PROXY_SEPARATE_RUN
    PS_ToWUDQueue = 5,
#endif
    PS_STOP = PQ_STOP} queue_events_t;


void init_queues();
void erase_queues();
//return queue ptr or null if no queue
pu_queue_t* pt_get_gueue(int que_number);

#endif //PRESTO_PT_QUEUES_H
