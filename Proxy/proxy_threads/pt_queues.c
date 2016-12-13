//
// Created by gsg on 06/12/16.
//

#include <pc_settings.h>
#include "pt_queues.h"


static pu_queue_t* qu_arr[PS_MAX_QUEUE-PS_MIN_QUEUE+1];

void init_queues() {
    pu_queues_init(PS_MAX_QUEUE-PS_MIN_QUEUE+1);
    for(unsigned int i = 0; i < PS_MAX_QUEUE-PS_MIN_QUEUE+1; i++) {
        qu_arr[i] = pu_queue_create(pc_getQueuesRecAmt(),i+PS_MIN_QUEUE);
    }
}
void erase_queues() {
    for(unsigned int i = 0; i < PS_MAX_QUEUE-PS_MIN_QUEUE+1; i++) {
        pu_queue_erase(qu_arr[i+PS_MIN_QUEUE]);
    }
    pu_queues_destroy();
}
//return queue ptr or null if no queue
pu_queue_t* pt_get_gueue(int que_number) {
    if(que_number < PS_MIN_QUEUE) return NULL;
    if(que_number >PS_MAX_QUEUE) return NULL;
    return qu_arr[que_number-PS_MIN_QUEUE];
}