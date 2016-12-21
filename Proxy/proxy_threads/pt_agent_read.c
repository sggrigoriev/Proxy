//
// Created by gsg on 06/12/16.
//
#include <pthread.h>
#include <aio.h>
#include <string.h>
#include <errno.h>
#include <pt_tcp_utl.h>

#include "pc_defaults.h"
#include "pu_logger.h"
#include "pt_queues.h"
#include "pt_agent_write.h"
#include "pt_agent_read.h"
#include "pt_main_agent.h"

#define PT_THREAD_NAME "AGENT_READ"

////////////////////////////
static pthread_t id;
static pthread_attr_t attr;

static struct aiocb cb;
static const struct aiocb* cb_list[1];
static char out_buf[PROXY_MAX_MSG_LEN] = {0};
static char in_buf[PROXY_MAX_MSG_LEN] = {0};
static char ass_buf[PROXY_MAX_MSG_LEN*2] = {0};
static struct timespec agent_rd_timeout = {1,0};

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

//Init section
    pt_tcp_assembling_buf_t as_buf = {ass_buf, sizeof(ass_buf), 0};

    to_main = pt_get_gueue(PS_FromAgentQueue);

    memset(&cb, 0, sizeof(struct aiocb));
    cb.aio_buf = in_buf;
    cb.aio_fildes = read_socket;
    cb.aio_nbytes = sizeof(in_buf)-1;
    cb.aio_offset = 0;

    memset(&cb_list, 0, sizeof(cb_list));
    cb_list[0] = &cb;

    if (aio_read(&cb) < 0) {                 //start read operation
        pu_log(LL_ERROR, "%s: aio_read: start operation failed %d %s", PT_THREAD_NAME, errno, strerror(errno));
        set_stop_agent_children();
    }

    while(!is_childs_stop()) {
        ssize_t bytes_read;
        if(aio_suspend(cb_list, 1, &agent_rd_timeout)) {    //wait until the read
            continue;
        }
        if(bytes_read = aio_return(&cb), bytes_read <= 0) {
            pu_log(LL_ERROR, "%s. Read op failed %d %s. Reconnect", PT_THREAD_NAME, errno, strerror(errno));
            set_stop_agent_children();
            continue;
        }
        if(pt_tcp_get(in_buf, bytes_read, &as_buf)) {
            while(pt_tcp_assemble(out_buf, sizeof(out_buf), &as_buf)) {     //Reag all fully incoming messages
                pu_queue_push(to_main, out_buf, strlen(out_buf)+1);
                pu_log(LL_INFO, "%s: message sent: %s", PT_THREAD_NAME, out_buf);
            }
        }
        else {
            pu_log(LL_ERROR, "%s: incoming mesage too large: %lu vs %lu. Ignored", PT_THREAD_NAME, bytes_read, sizeof(ass_buf));
            continue;
        }
        if (aio_read(&cb) < 0) {                                         //Run read operation again
            pu_log(LL_ERROR, "read proc: aio_read: start operation failed %d %s", errno, strerror(errno));
            set_stop_agent_children();;
        }
    }
    pu_log(LL_INFO, "%s is finished", PT_THREAD_NAME);
    pthread_exit(NULL);
}

