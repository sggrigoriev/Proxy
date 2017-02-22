//
// Created by gsg on 07/12/16.
//

#include "wc_settings.h"
#include "wt_queues.h"

static pu_queue_t* qu_arr[WT_MAX_QUEUE-WT_MIN_QUEUE+1];
static pthread_mutex_t  own_mutex;

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
//return queue ptr or null if no queue
pu_queue_t* wt_get_gueue(int que_number) {
    if(que_number < WT_MIN_QUEUE) return NULL;
    if(que_number >WT_MAX_QUEUE) return NULL;

    pthread_mutex_lock(&own_mutex);
    pu_queue_t* ret = qu_arr[que_number-WT_MIN_QUEUE];
    pthread_mutex_unlock(&own_mutex);

    return ret;
}