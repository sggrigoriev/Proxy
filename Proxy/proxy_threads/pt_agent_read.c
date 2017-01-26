//
// Created by gsg on 06/12/16.
//
#include <pthread.h>
#include <string.h>
#include <errno.h>

#include "lib_tcp.h"
#include "pc_defaults.h"
#include "pu_logger.h"
#include "pt_queues.h"
#include "pt_agent_read.h"
#include "pt_main_agent.h"

#define PT_THREAD_NAME "AGENT_READ"

////////////////////////////
static pthread_t id;
static pthread_attr_t attr;


static char out_buf[PROXY_MAX_MSG_LEN] = {0};

int read_socket;
static pu_queue_t* to_main;

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

    lib_tcp_conn_t* all_conns = lib_tcp_init_conns(1, PROXY_MAX_MSG_LEN, PROXY_MAX_MSG_LEN*2);
    if(!all_conns) {
        pu_log(LL_ERROR, "%s: memory allocation error.", PT_THREAD_NAME);
        set_stop_agent_children();
        goto allez;      //Allez kaputt
    }
    if(!lib_tcp_add_new_conn(read_socket, all_conns)) {
        pu_log(LL_ERROR, "%s: new incoming connection exeeds max amount. Aborted", PT_THREAD_NAME);
        set_stop_agent_children();
        goto allez;
    }

    while(!is_childs_stop()) {
        lib_tcp_rd_t *conn = lib_tcp_read(all_conns, 1); //connection removed inside
        if (!conn) {
            pu_log(LL_ERROR, "%s. Read op failed %d %s. Reconnect", PT_THREAD_NAME, errno, strerror(errno));
            set_stop_agent_children();
            break;
        }
        if (conn == LIB_TCP_READ_TIMEOUT) {
            continue;   //timeout
        }
        if (conn == LIB_TCP_READ_MSG_TOO_LONG) {
            pu_log(LL_ERROR, "%s: incoming mesage too large: %lu vs %lu. Ignored", PT_THREAD_NAME, conn->in_buf.len, conn->ass_buf.size);
            continue;
        }
        if (conn == LIB_TCP_READ_NO_READY_CONNS) {
            pu_log(LL_ERROR, "%s: internal error - no ready sockets. Reconnect", PT_THREAD_NAME);
            set_stop_agent_children();
            break;
        }
        while (lib_tcp_assemble(conn, out_buf, sizeof(out_buf))) {     //Reag all fully incoming messages
            pu_queue_push(to_main, out_buf, strlen(out_buf) + 1); //TODO Mlevitin

            pu_log(LL_INFO, "%s: message sent: %s", PT_THREAD_NAME, out_buf);
        }
    }
    allez:
    lib_tcp_destroy_conns(all_conns);
    pu_log(LL_INFO, "%s is finished", PT_THREAD_NAME);
    pthread_exit(NULL);
}

