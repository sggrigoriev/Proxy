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
    Created by gsg on 07/12/16.
*/
#include <pthread.h>
#include <string.h>
#include <errno.h>

#include "pr_commands.h"
#include "lib_tcp.h"

#include "wt_queues.h"
#include "wc_defaults.h"
#include "wc_settings.h"
#include "wt_agent_proxy_read.h"


#define PT_THREAD_NAME "AGENT_PROXY_READ"

/**************************************************************
 * Local data
 */
static pthread_t id;
static pthread_attr_t attr;

static char out_buf[WC_MAX_MSG_LEN];    /* Buffer to receive messages */

static volatile int stop;               /* stop thread flag */

static pu_queue_t* proxy_commands;      /* Transport to pass the data to the main WUD thread */

/* Local functions definition */

/**************************************************************
 * Thread function
 */
static void* ap_reader(void* params);

/**************************************************************
 * Public functions implementation
 */

int wt_start_agent_proxy_read() {
    if(pthread_attr_init(&attr)) return 0;
    if(pthread_create(&id, &attr, &ap_reader, NULL)) return 0;
    return 1;
}

void wt_set_stop_agent_proxy_read() {
    stop = 1;
}

void wt_stop_agent_proxy_read() {
    void *ret;

    wt_set_stop_agent_proxy_read();
    pthread_join(id, &ret);
    pthread_attr_destroy(&attr);
}

/**************************************************************
 * Local functions implementation
 */
static void* ap_reader(void* params) {

    proxy_commands = wt_get_gueue(WT_to_Main);

    while(!stop) {
       int server_socket = lib_tcp_get_server_socket(wc_getWUDPort());
        if(server_socket < 0) {
            pu_log(LL_ERROR, "%s: unable to bind to port %d. %d %s", PT_THREAD_NAME, wc_getWUDPort(), errno, strerror(errno));
            stop = 1;
            break;      /* Allez kaputt */
        }
        lib_tcp_conn_t* all_conns = lib_tcp_init_conns(PR_CHILD_SIZE, WC_MAX_MSG_LEN-LIB_HTTP_HEADER_SIZE);
        if(!all_conns) {
            pu_log(LL_ERROR, "%s: memory allocation error.", PT_THREAD_NAME);
            stop = 1;
            break;      /* Allez kaputt */
        }
        while(!stop) {

            int rd_socket = lib_tcp_listen(server_socket, 2);
            if(rd_socket < 0) {
                pu_log(LL_ERROR, "%s: listen error. %d %s", PT_THREAD_NAME, errno, strerror(errno));
                close(server_socket);
                break;      /*  Go to bing again */
            }
            if(!rd_socket) {    /* timeout */
                continue;
            }
/* Got valid socket */
            if(!lib_tcp_add_new_conn(rd_socket, all_conns)) {
                pu_log(LL_WARNING, "%s: new incoming connection exeeds max amount. Ignored", PT_THREAD_NAME);
                close(rd_socket);
            }
            else {
                pu_log(LL_INFO, "%s: got incoming connection", PT_THREAD_NAME);
            }
            while(!stop && (all_conns->sa_max_size == all_conns->sa_size)) { /* all connections are in use - no need go to listen */
                lib_tcp_rd_t *conn = lib_tcp_read(all_conns, 1); /* connection removed inside */
                if (!conn) {
                    pu_log(LL_ERROR, "%s: read error. Reconnect. %d %s", PT_THREAD_NAME, errno, strerror(errno));
                    break;
                }
                if (conn == LIB_TCP_READ_TIMEOUT) {
                    continue;   /* timeout */
                }
                if (conn == LIB_TCP_READ_MSG_TOO_LONG) {
                    pu_log(LL_ERROR, "%s: incoming message too long. Ignored", PT_THREAD_NAME);
                    continue;
                }
                if (conn == LIB_TCP_READ_NO_READY_CONNS) {
                    pu_log(LL_ERROR, "%s: internal error - no ready sockets. Ignored", PT_THREAD_NAME);
                    break;
                }
                if(conn == LIB_TCP_READ_EOF) {
                    pu_log(LL_ERROR, "%s. Read op failed. Nobody on remote side (EOF). Reconnect", PT_THREAD_NAME);
                    break;
                }
                while (lib_tcp_assemble(conn, out_buf, sizeof(out_buf))) {      /* Read all fully incoming messages... */
                    pu_queue_push(proxy_commands, out_buf, strlen(out_buf) + 1);
                    pu_log(LL_INFO, "%s: sent to wud main: %s", PT_THREAD_NAME, out_buf);
                }
            }
        }
        close(server_socket);
        lib_tcp_destroy_conns(all_conns);
    }
    pu_log(LL_INFO, "%s is finished", PT_THREAD_NAME);
    pthread_exit(NULL);
}