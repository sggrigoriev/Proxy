/*
 *  Copyright 2017 People Power Company
 *
 *  This code was developed with funding from People Power Company
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */
/*
    Created by gsg on 06/12/16.
*/
#include <pthread.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>

#include "lib_tcp.h"
#include "pu_logger.h"

#include "pc_defaults.h"
#include "pc_settings.h"
#include "pt_agent_read.h"
#include "pt_agent_write.h"

#include "pt_main_agent.h"

#define PT_THREAD_NAME "AGENT_MAIN"

/*************************************************************************
 * Local data
 */
static pthread_t id;
static pthread_attr_t attr;

static volatile int stop;       /* Thread stop flag */
static volatile int chids_stop; /* Children (read/write) stop flag */

/* Tread function: creates server socket, listen for Agent connection; duplicate and have the writable socket; wait until children dead*/
static void* agent_main(void* params);

/*********************************************************************************
 * Public functions
*/

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

/**************************************************************************************
 * Local functione implementation
*/
static void* agent_main(void* params) {
    stop = 0;

    while(!stop) {
        int read_socket = -1;
        int write_socket = -1;
        int server_socket = -1;
        chids_stop = 0;

        if(server_socket = lib_tcp_get_server_socket(pc_getAgentPort(), pc_getProxyListenIP()), server_socket < 0) {
            pu_log(LL_ERROR, "%s: unable to bind to the port %d. %d %s. Exiting.", PT_THREAD_NAME, pc_getAgentPort(), errno, strerror(errno));
            stop = 1;
            break;
        }
        do {
            if (read_socket = lib_tcp_listen(server_socket, 1), read_socket < 0) {
                pu_log(LL_ERROR, "%s: listen error. %d %s. Exiting", PT_THREAD_NAME, errno, strerror(errno));
                lib_tcp_client_close(server_socket);
                server_socket = -1;
                break;      /* Go to bing again */
            }
            if (!read_socket) {    /* timeout */
                continue;
            }
        }
        while (!read_socket);  /* until the timeout */

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

/* If we're here one of threads is dead. Let's make happy another one... */
        stop_agent_read();
        stop_agent_write();

/* Read & write sockets will be closed inside the agent_read & agent_write */
        lib_tcp_client_close(server_socket);
        pu_log(LL_WARNING, "Agent read/write threads restart");
    }
    pthread_exit(NULL);
}