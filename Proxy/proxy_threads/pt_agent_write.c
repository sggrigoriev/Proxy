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
#include "pt_main_agent.h"
#include "pt_agent_write.h"


#define PT_THREAD_NAME "AGENT_WRITE"

////////////////////////////
static pthread_t id;
static pthread_attr_t attr;
static volatile int stop;   //one stop for write and read agent threads

static char out_buf[PROXY_MAX_MSG_LEN];

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
                    ssize_t ret;
                    while(ret = lib_tcp_write(write_socket, out_buf, len, 1), !ret&&!is_childs_stop());  //run until the timeout
                    if(is_childs_stop()) break; // goto reconnect
                    if(ret < 0) { //op start failed
                        pu_log(LL_ERROR, "%s. Write op start failed %d %s. Reconnect", PT_THREAD_NAME, errno, strerror(errno));
                        set_stop_agent_children();
                        break;
                    }
                    pu_log(LL_DEBUG, "%s: written: %s", PT_THREAD_NAME, out_buf);
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
    lib_tcp_client_close(write_socket);
    pthread_exit(NULL);
}

