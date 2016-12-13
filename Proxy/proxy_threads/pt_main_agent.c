//
// Created by gsg on 06/12/16.
//
#include <pthread.h>
#include <string.h>
#include <pc_settings.h>
#include <pt_tcp_utl.h>
#include <errno.h>

#include "pc_defaults.h"
#include "pu_logger.h"
#include "pt_queues.h"
#include "pt_main_agent.h"
#include "pt_agent_read.h"
#include "pt_agent_write.h"

#define PT_THREAD_NAME "AGENT_MAIN"

////////////////////////////
static pthread_t id;
static pthread_attr_t attr;
static volatile int stop;
static volatile int chids_stop;


static void* agent_main(void* params);

int start_agent_main() {
    if(pthread_attr_init(&attr)) return 0;
    if(pthread_create(&id, &attr, &agent_main, NULL)) return 0;
    return 1;
}

void stop_agent_main() {
    void *ret;

    set_stop_agent_main();
    pthread_join(id, &ret);
    pthread_attr_destroy(&attr);
}

void set_stop_agent_main() {
    stop = 1;
}

void set_stop_agent_children() {
    chids_stop = 1;
}
int is_childs_stop() {
    return chids_stop;
}

static void* agent_main(void* params) {
    stop = 0;

    while(!stop) {
        pt_tcp_rw_t rw = {-1, -1, -1};
        chids_stop = 0;

        if(pt_tcp_server_connect(pc_getAgentPort(), &rw)) {
            pu_log(LL_DEBUG, "%s: read_socket = %d, write_socket = %d", PT_THREAD_NAME, rw.rd_socket, rw.wr_socket);
            if(!start_agent_read(rw.rd_socket)) {
                pu_log(LL_ERROR, "%s: Creating %s failed: %s", PT_THREAD_NAME, "AGENT_READ", strerror(errno));
                break;
            }
            pu_log(LL_INFO, "%s: started", "AGENT_READ");

            if(!start_agent_write(rw.wr_socket)) {
                pu_log(LL_ERROR, "%s: Creating %s failed: %s", PT_THREAD_NAME, "AGENT_WRITE", strerror(errno));
                break;
            }
            pu_log(LL_INFO, "%s: started", "AGENT_WRITE");

//If we're here one of threads is dead. Let's make happy another one...
            stop_agent_read();
            stop_agent_write();

            pt_tcp_shutdown_rw(&rw);
            pu_log(LL_WARNING, "Agent read/write threads restart");
        }
        else {
            pu_log(LL_ERROR, "%s: connection error %d %s", PT_THREAD_NAME, errno, strerror(errno));
            sleep(1);
        }
    }
    pthread_exit(NULL);
}