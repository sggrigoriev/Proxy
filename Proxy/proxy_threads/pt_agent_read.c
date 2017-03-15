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

#include "lib_tcp.h"
#include "pu_logger.h"
#include "pt_queues.h"

#include "pc_defaults.h"
#include "pt_main_agent.h"
#include "pt_agent_read.h"

#define PT_THREAD_NAME "AGENT_READ"

/***************************************************************************************************
 * Local data
 */
static pthread_t id;
static pthread_attr_t attr;

static char out_buf[LIB_HTTP_MAX_MSG_SIZE] = {0};   /* buffer to receive the data */

int read_socket;                            /* socket to read from  */
static pu_queue_t* to_main;                 /* main queue pointer - local transport */

/* Thread function. Reads info, assemple it to the buffer and forward to the main thread by queue */
static void* agent_read(void* params);

int start_agent_read(int socket) {
    read_socket = socket;
    if(pthread_attr_init(&attr)) return 0;
    if(pthread_create(&id, &attr, &agent_read, &read_socket)) return 0;
    return 1;
}

void stop_agent_read() {
    void *ret;

    pthread_join(id, &ret);
    pthread_attr_destroy(&attr);

    set_stop_agent_children();
}

static void* agent_read(void* params) {

    to_main = pt_get_gueue(PS_FromAgentQueue);

    lib_tcp_conn_t* all_conns = lib_tcp_init_conns(1, PROXY_MAX_MSG_LEN-LIB_HTTP_HEADER_SIZE, PROXY_MAX_MSG_LEN*2);
    if(!all_conns) {
        pu_log(LL_ERROR, "%s: memory allocation error.", PT_THREAD_NAME);
        set_stop_agent_children();
        goto allez;      /* Allez kaputt */
    }
    if(!lib_tcp_add_new_conn(read_socket, all_conns)) {
        pu_log(LL_ERROR, "%s: new incoming connection exeeds max amount. Aborted", PT_THREAD_NAME);
        set_stop_agent_children();
        goto allez;
    }

    while(!is_childs_stop()) {
        lib_tcp_rd_t *conn = lib_tcp_read(all_conns, 1); /* connection removed inside */
        if (!conn) {
            pu_log(LL_ERROR, "%s. Read op failed %d %s. Reconnect", PT_THREAD_NAME, errno, strerror(errno));
            set_stop_agent_children();
            break;
        }
        if(conn == LIB_TCP_READ_EOF) {
            pu_log(LL_ERROR, "%s. Read op failed. Nobody on remote side (EOF). Reconnect", PT_THREAD_NAME);
            set_stop_agent_children();
            break;
        }
        if (conn == LIB_TCP_READ_TIMEOUT) {
            continue;   /* timeout */
        }
        if (conn == LIB_TCP_READ_MSG_TOO_LONG) {
            pu_log(LL_ERROR, "%s: incoming mesage too large. Ignored", PT_THREAD_NAME);
            continue;
        }
        if (conn == LIB_TCP_READ_NO_READY_CONNS) {
            pu_log(LL_ERROR, "%s: internal error - no ready sockets. Reconnect", PT_THREAD_NAME);
            set_stop_agent_children();
            break;
        }
        while (lib_tcp_assemble(conn, out_buf, sizeof(out_buf))) {     /* Read all fully incoming messages */
            pu_queue_push(to_main, out_buf, strlen(out_buf) + 1); /*TODO Mlevitin */

            pu_log(LL_INFO, "%s: message sent: %s", PT_THREAD_NAME, out_buf);
        }
    }
    allez:
    lib_tcp_destroy_conns(all_conns);
    pu_log(LL_INFO, "%s is finished", PT_THREAD_NAME);
    pthread_exit(NULL);
}

