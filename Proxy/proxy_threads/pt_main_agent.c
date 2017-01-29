//
// Created by gsg on 06/12/16.
//
#include <pthread.h>
#include <string.h>
#include <errno.h>

#include <pc_settings.h>
#include "lib_tcp.h"
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
        int read_socket = -1;
        int write_socket = -1;
        int server_socket = -1;
        chids_stop = 0;

        if(server_socket = lib_tcp_get_server_socket(pc_getAgentPort()), server_socket < 0) {
            pu_log(LL_ERROR, "%s: unable to bind. %d %s", PT_THREAD_NAME, errno, strerror(errno));
            stop = 1;
            break;
        }
        do {
            if (read_socket = lib_tcp_listen(server_socket, 1), read_socket < 0) {
                pu_log(LL_ERROR, "%s: listen error. %d %s", PT_THREAD_NAME, errno, strerror(errno));
                close(server_socket);
                break;      // Go to bing again
            }
            if (!read_socket) {    //timeout
                continue;
            }
        }
        while (!read_socket);  // until the timeout

        write_socket = dup(read_socket);

        if(!start_agent_read(read_socket)) {
            pu_log(LL_ERROR, "%s: Creating %s failed: %s", PT_THREAD_NAME, "AGENT_READ", strerror(errno));
            break;
        }
        pu_log(LL_INFO, "%s: started", "AGENT_READ");

        if(!start_agent_write(write_socket)) {
            pu_log(LL_ERROR, "%s: Creating %s failed: %s", PT_THREAD_NAME, "AGENT_WRITE", strerror(errno));
            break;
        }
        pu_log(LL_INFO, "%s: started", "AGENT_WRITE");

//If we're here one of threads is dead. Let's make happy another one...
        stop_agent_read();
        stop_agent_write();

        lib_tcp_client_close(read_socket);
        lib_tcp_client_close(write_socket);
        pu_log(LL_WARNING, "Agent read/write threads restart");
    }
    pthread_exit(NULL);
}