//
// Created by gsg on 07/12/16.
//

#ifndef PRESTO_WT_QUEUES_H
#define PRESTO_WT_QUEUES_H

#include "pu_queue.h"

#define WT_MIN_QUEUE WT_to_Main
#define WT_MAX_QUEUE WT_to_Cloud

typedef enum {WT_Timeout = PQ_TIMEOUT,
    WT_to_Main = 1, WT_to_Cloud = 2,
    WT_STOP = PQ_STOP} wt_queue_events_t;


void wt_init_queues();
void wt_erase_queues();
//return queue ptr or null if no queue
pu_queue_t* wt_get_gueue(int que_number);


#endif //PRESTO_WT_QUEUES_H
