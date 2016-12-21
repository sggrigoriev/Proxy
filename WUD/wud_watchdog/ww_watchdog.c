//
// Created by gsg on 01/12/16.
//
#include <pthread.h>
#include <string.h>
#include <pr_commands.h>


#include "pu_logger.h"
#include "pr_commands.h"
#include "wm_childs_info.h"
#include "ww_watchdog.h"

typedef struct {
    time_t last_update;
    unsigned int restart_delta;  // restart if restart_delta+last_update < time(NULL)

} ww_timer_t;



static ww_timer_t WD_TIMERS[WA_SIZE];
static pthread_mutex_t lock;

void ww_waitchdog_init() {
    for(unsigned int i = 0; i < WA_SIZE; i++) {
        WD_TIMERS[i].restart_delta = pr_child_get_child_to(i);
        WD_TIMERS[i].last_update = time(NULL);
    }
}
void ww_watchdog_destroy() {
    memset(WD_TIMERS, 0, WA_SIZE*sizeof(ww_timer_t));
}
void ww_watchdog_update(const char* who) {
    pthread_mutex_lock(&lock);
    wa_child_t idx = pr_string_2_chld(who);
    if((idx >= 0) && (idx < WA_SIZE)) {
        WD_TIMERS[idx].last_update = time(NULL);
    }
    pthread_mutex_unlock(&lock);
}

char* ww_watchdog_analyze() {
    pthread_mutex_lock(&lock);
    char *buf = NULL;
    time_t timestamp = time(NULL);
    for(unsigned int i = 0; i < WA_SIZE; i++ ) {
        if((WD_TIMERS[i].last_update + WD_TIMERS[i].restart_delta) >= timestamp)
            WD_TIMERS[i].last_update  = timestamp;
        else {
            pr_message_t msg;
            msg.restart_child.message_type = PR_COMMAND;
            msg.restart_child.cmd = PR_RESTART_CHILD;
            msg.restart_child.process_idx = (wa_child_t)i;

            pr_make_message(&buf, msg);
            pr_erase_msg(msg);  //non needed but things could change...
        }
    }
    pthread_mutex_unlock(&lock);
    return buf;
}

