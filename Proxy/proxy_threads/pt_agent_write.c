//
// Created by gsg on 06/12/16.
//
#include <aio.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>

#include "pc_defaults.h"
#include "pu_logger.h"
#include "pt_queues.h"
#include "pt_main_agent.h"
#include "pt_agent_write.h"


#define PT_THREAD_NAME "AGENT_WRITE"

////////////////////////////
static pthread_t id;
static pthread_attr_t attr;
static volatile int stop;   //one stop for write and read agent threads

static struct aiocb cb;
static const struct aiocb* cb_list[1];
static char out_buf[PROXY_MAX_MSG_LEN*2];
static struct timespec agent_wr_timeout = {1,0};

static int write_socket;
static pu_queue_t* from_main;

static void* agent_write(void* params);

int start_agent_write(int socket) {
    write_socket = socket;
    if(pthread_attr_init(&attr)) return 0;
    if(pthread_create(&id, &attr, &agent_write, NULL)) return 0;
    return 1;
}

void stop_agent_write() {
    void *ret;
    pthread_join(id, &ret);
    pthread_attr_destroy(&attr);

    set_stop_agent_children();
}

static void* agent_write(void* params) {
    from_main = pt_get_gueue(PS_ToAgentQueue);

//Init section
    memset(&cb, 0, sizeof(struct aiocb));
    cb.aio_buf = out_buf;
    cb.aio_fildes = write_socket;
    cb.aio_nbytes = sizeof(out_buf)-1;
    cb.aio_offset = 0;

    memset(&cb_list, 0, sizeof(cb_list));
    cb_list[0] = &cb;
//Queue events init
    pu_queue_event_t events;
    events = pu_add_queue_event(pu_create_event_set(), PS_ToAgentQueue);

//Main loop
//    pu_log(LL_DEBUG,"%s: is_stop_agent_write_agent_read() = %d", PT_THREAD_NAME, is_stop_agent_write_agent_read());
    while(!is_childs_stop()) {
        pu_queue_event_t ev;

        switch(ev=pu_wait_for_queues(events, DEFAULT_AGENT_THREAD_TO_SEC)) {
            case PS_ToAgentQueue: {
                size_t len = sizeof(out_buf);
                while (pu_queue_pop(from_main, out_buf, &len)) {
                    //Prepare write operation
                    cb.aio_nbytes = len + 1;
                    //Start write operation
                    int ret = aio_write(&cb);
                    if(ret != 0) { //op start failed
                        pu_log(LL_ERROR, "%s. Write op start failed %d %s. Reconnect", PT_THREAD_NAME, errno, strerror(errno));
                        set_stop_agent_children();
                    }
                    else {
                        int wait_ret = aio_suspend(cb_list, 1, &agent_wr_timeout); //wait until the write
                        if(wait_ret == 0) {                                         //error in write
                            ssize_t bytes_written = aio_return(&cb);
                            if(bytes_written <= 0) {
                                pu_log(LL_ERROR, "%s. Write op finish failed %d %s. Reconnect", PT_THREAD_NAME, errno, strerror(errno));
                                set_stop_agent_children();
                            }
                        }
                        else if (wait_ret == EAGAIN) {   //Timeout
                            pu_log(LL_DEBUG, "%s: timeout", PT_THREAD_NAME);
                        }
                        else {
//                pu_log(LL_WARNING, "Client write was interrupted");
                        }
                    }
                    len = sizeof(out_buf);
                }
                break;
            }
            case PS_Timeout:
//                          pu_log(LL_WARNING, "agent_thread: timeout on waiting from server queue");
                break;
            case PS_STOP:
                set_stop_agent_children();
                pu_log(LL_INFO, "%s: received STOP event. Terminated", PT_THREAD_NAME);
                break;
            default:
                pu_log(LL_ERROR, "%s: Undefined event %d on wait!", PT_THREAD_NAME, ev);
                break;
        }
    }
    pu_log(LL_INFO, "%s is finished", PT_THREAD_NAME);
    pthread_exit(NULL);
}

