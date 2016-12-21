//
// Created by gsg on 07/12/16.
//

#ifndef PRESTO_WT_QUEUES_H
#define PRESTO_WT_QUEUES_H

#include "pu_queue.h"

#define WT_MIN_QUEUE WT_ProxyAgentMessage
#define WT_MAX_QUEUE WT_AlertsToCloud

typedef enum {WT_Timeout = PQ_TIMEOUT,
    WT_ProxyAgentMessage = 1, WT_WatchDogQueue = 2, WT_MonitorAlerts = 3, WT_FWInfo = 4, WT_AlertsToCloud = 5,
    WT_STOP = PQ_STOP} wt_queue_events_t;


void wt_init_queues();
void wt_erase_queues();
//return queue ptr or null if no queue
pu_queue_t* wt_get_gueue(int que_number);


#endif //PRESTO_WT_QUEUES_H
