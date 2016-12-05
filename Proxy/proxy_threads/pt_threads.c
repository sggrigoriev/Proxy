//
// Created by gsg on 03/11/16.
//
#include <pthread.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <unistd.h>
#include <aio.h>

#include "pc_defaults.h"
#include "pc_settings.h"
#include "pt_http_utl.h"
#include "pu_queue.h"

////////////////////////////////////////////
// Global params
static pu_queue_t* to_server;
static pu_queue_t* from_server;
static pu_queue_t* to_agent;
static pu_queue_t* from_agent;

//Total stop
static volatile int finish = 0;

//Queue events defines
typedef enum {PS_Timeout = PQ_TIMEOUT,
    PS_FromAgentQueue = 1, PS_ToServerQueue = 2, PS_FromServerQueue = 3, PS_ToAgentQueue = 4,
    PS_STOP = PQ_STOP} queue_events_t;
static const unsigned int Q_AMOUNT = 4;

//All local threads prototypes
static void* server_read_thread(void* dummy_params);
static void* server_write_thread(void* dummy_params);
static void* main_agent_thread(void *dummy_params);
static void* agent_read_proc(void* socket);
static void* agent_write_proc(void* socket);

//Main thread mgmt functions
static int main_thread_startup();
static void main_thread_shutdown();

///////////////////////////////////////////////////////////////////////////////
//Main proxy thread
//
//The only main thread's buffer!
static pu_queue_msg_t mt_msg[PROXY_MAX_MSG_LEN];
int pt_main_thread() { //Starts the main thread.
    pu_queue_event_t events;

    if(!main_thread_startup()) {
        pu_log(LL_ERROR, "Main Proxy thread initiation failed. Abort.");
        return 0;
    }
    events = pu_add_queue_event(pu_create_event_set(), PS_FromAgentQueue);
    events = pu_add_queue_event(events, PS_FromServerQueue);

    while(!finish) {
        size_t len = sizeof(mt_msg);
        pu_queue_event_t ev;

        switch (ev=pu_wait_for_queues(events, DEFAULT_MAIN_THREAD_TO_SEC)) {
            case PS_Timeout:
                pu_log(LL_DEBUG, "main_thread: TIMEOUT");
                break;
            case PS_FromServerQueue:
                while(pu_queue_pop(from_server, mt_msg, &len)) {
                    pu_queue_push(to_agent, mt_msg, len);
                    pu_log(LL_DEBUG, "maim_thread: msg %d bytes was sent to agent: %s ", len, mt_msg);
                    len = sizeof(mt_msg);
                }
                break;
            case PS_FromAgentQueue:
                while(pu_queue_pop(from_agent, mt_msg, &len)) {
                    pu_queue_push(to_server, mt_msg, len);
                    pu_log(LL_DEBUG, "main_thread: msg %s was sent to server_write", mt_msg);
                    len = sizeof(mt_msg);
                }
                break;
            case PS_STOP:
                finish = 1;
                pu_log(LL_INFO, "main_thread received STOP event. Terminated");
                break;
            default:
                pu_log(LL_ERROR, "main_thread: Undefined event %d on wait (from agent / from server)!", ev);
                break;
        }
    }
    main_thread_shutdown();
    return 0;
}
///////////////////////////////////////////////////////////////////////////////
//Local functions defenition
//
static void* server_read_thread(void* dummy_params) {
    char *buf = NULL;

    if (!pt_http_read_init()) {
        pu_log(LL_ERROR, "server_read_thread: Cloud info read initiation failed. Stop.");
        pthread_exit(NULL);
    }

//Main read loop
    while(!finish) {
        int rc = 0;
        while((rc <= 0) && !finish) {
            rc = pt_http_read(&buf);
        }
        if(!finish) {
            pu_queue_push(from_server, buf, (unsigned int)rc + 1);  //NB! Possibly needs to split info to 0-terminated strings!
            pu_log(LL_INFO, "Received from cloud: %s", buf);
        }
        else {
            pu_log(LL_INFO, "server_read_thread STOP. Terminated");
        }
    }
    pt_http_read_destroy();
    pthread_exit(NULL);
}
////////////////////////////////////////////////////////
//THe only write_thread buffer!
static pu_queue_msg_t wt_msg[PROXY_MAX_MSG_LEN];
static void* server_write_thread(void* dummy_params) {
    pu_queue_event_t events;

    if(!pt_http_write_init()) {
        pu_log(LL_ERROR, "server_write_thread: Cloud info write initiation failed. Stop.");
        pthread_exit(NULL);
    }

    events = pu_add_queue_event(pu_create_event_set(), PS_ToServerQueue);

//Main write loop
    while(!finish) {
        pu_queue_event_t ev;
        switch (ev = pu_wait_for_queues(events, DEFAULT_SERVER_WRITE_THREAD_TO_SEC)) {
            case PS_ToServerQueue: {
                size_t len = sizeof(wt_msg);
                while (pu_queue_pop(to_server, wt_msg, &len)) {
                    pu_log(LL_DEBUG, "Got from from main by server_write_thread: %s", wt_msg);

                    int rc = 0;
                    char *resp = NULL;
                    size_t resp_len = 0;
                    while ((rc <= 0) && !finish) {
                        rc = pt_http_write(wt_msg, len, &resp, &resp_len);
                    }
                    if (!finish) {
                        pu_log(LL_INFO, "Sent to cloud: %s", wt_msg);
                        if (resp_len > 0) {
                            pu_log(LL_INFO, "Answer from cloud: %s", wt_msg);
                            pu_queue_push(from_server, resp, resp_len);
                        }
                        len = sizeof(wt_msg);
                    }
                }
                break;
            }
            case PS_Timeout:
                pu_log(LL_WARNING, "server_write_thread: timeout on waiting to server queue");
                break;
            case PS_STOP:
                finish = 1;
                pu_log(LL_INFO, "server_write_thread received STOP event. Terminated");
                break;
            default:
                pu_log(LL_ERROR, "server_write_thread: Undefined event %d on wait (to server)!", ev);
                break;
        }
    }
    pt_http_write_destroy();
    pthread_exit(NULL);
}
////////////////////////////////////
//The common data for main_agent_thread and his children
static volatile int agent_rw_stop = 0;
/////////////////////////////////////////////////////////
//The only agent_read_thread data
static struct aiocb agent_rd_cb;
static const struct aiocb* agent_rd_cb_list[1];
static char agent_rd_out_buf[PROXY_MAX_MSG_LEN];
static char agent_rd_buf[PROXY_MAX_MSG_LEN*2];
static char agent_rd_ass_buf[PROXY_MAX_MSG_LEN*2];
static struct timespec agent_rd_timeout = {1,0};
//
static void* agent_read_proc(void* socket) {
    int read_socket = *(int *)socket;

//Init section
    pt_tcp_assembling_buf_t as_buf = {agent_rd_ass_buf, sizeof(agent_rd_ass_buf), 0};

    memset(&agent_rd_cb, 0, sizeof(struct aiocb));
    agent_rd_cb.aio_buf = agent_rd_buf;
    agent_rd_cb.aio_fildes = read_socket;
    agent_rd_cb.aio_nbytes = sizeof(agent_rd_buf)-1;
    agent_rd_cb.aio_offset = 0;

    memset(&agent_rd_cb_list, 0, sizeof(agent_rd_cb_list));
    agent_rd_cb_list[0] = &agent_rd_cb;

    if (aio_read(&agent_rd_cb) < 0) {                 //start read operation
        pu_log(LL_ERROR, "read proc: aio_read: start operation failed %d %s", errno, strerror(errno));
        agent_rw_stop = 1;
    }
    while(!agent_rw_stop) {
        int ret = aio_suspend(agent_rd_cb_list, 1, &agent_rd_timeout); //wait until the read
        if(ret == 0) {                                      //Read succeed
            ssize_t bytes_read = aio_return(&agent_rd_cb);
            if(bytes_read > 0) {
                pt_tcp_get(agent_rd_buf, bytes_read, &as_buf);
                while(pt_tcp_assemble(agent_rd_out_buf, sizeof(agent_rd_out_buf), &as_buf)) {     //Reag all fully incoming messages
                    pu_queue_push(to_server, agent_rd_out_buf, strlen(agent_rd_out_buf)+1);
                    pu_log(LL_INFO, "From agent: %s", agent_rd_out_buf);
                }
            }
            else {  //error in read
                pu_log(LL_ERROR, "Server. Read op failed %d %s. Reconnect", errno, strerror(errno));
                agent_rw_stop = 1;
            }
            if (aio_read(&agent_rd_cb) < 0) {                                         //Run read operation again
                pu_log(LL_ERROR, "read proc: aio_read: start operation failed %d %s", errno, strerror(errno));
                agent_rw_stop = 1;
            }
        }
        else if (ret == EAGAIN) {   //Timeout
            pu_log(LL_DEBUG, "Server read: timeout");
        }
        else {
//            pu_log(LL_WARNING, "Server read was interrupted. ");
        }
    }
    pu_log(LL_INFO, "Client's read is finished");
    pthread_exit(NULL);
}
/////////////////////////////////////////////////////////
//The only agent_write_thread data
static struct aiocb agent_wr_cb;
static const struct aiocb* agent_wr_cb_list[1];
static char agent_wr_buf[PROXY_MAX_MSG_LEN*2];
static struct timespec agent_wr_timeout = {1,0};
//
static void* agent_write_proc(void* socket) {
    int write_socket = *(int *)socket;
//Init section
    memset(&agent_wr_cb, 0, sizeof(struct aiocb));
    agent_wr_cb.aio_buf = agent_wr_buf;
    agent_wr_cb.aio_fildes = write_socket;
    agent_wr_cb.aio_nbytes = sizeof(agent_wr_buf)-1;
    agent_wr_cb.aio_offset = 0;

    memset(&agent_wr_cb_list, 0, sizeof(agent_wr_cb_list));
    agent_wr_cb_list[0] = &agent_wr_cb;
//Queue events init
    pu_queue_event_t events;
    events = pu_add_queue_event(pu_create_event_set(), PS_ToAgentQueue);

//Main loop
    while(!agent_rw_stop) {
        pu_queue_event_t ev;

        switch(ev=pu_wait_for_queues(events, DEFAULT_AGENT_THREAD_TO_SEC)) {
            case PS_ToAgentQueue: {
                size_t len = sizeof(agent_wr_buf);
                while (pu_queue_pop(to_agent, agent_wr_buf, &len)) {
                    //Prepare write operation
                     agent_wr_cb.aio_nbytes = len + 1;
                    //Start write operation
                    int ret = aio_write(&agent_wr_cb);
                    if(ret != 0) { //op start failed
                        pu_log(LL_ERROR, "Agent write thread. Write op start failed %d %s. Reconnect", errno, strerror(errno));
                        agent_rw_stop = 1;
                    }
                    else {
                        int wait_ret = aio_suspend(agent_wr_cb_list, 1, &agent_wr_timeout); //wait until the write
                        if(wait_ret == 0) {                                         //error in write
                            ssize_t bytes_written = aio_return(&agent_wr_cb);
                            if(bytes_written <= 0) {
                                pu_log(LL_ERROR, "Agent write thread. Write op finish failed %d %s. Reconnect", errno, strerror(errno));
                                agent_rw_stop = 1;
                            }
                        }
                        else if (wait_ret == EAGAIN) {   //Timeout
                            pu_log(LL_DEBUG, "Agent write thread: timeout");
                        }
                        else {
//                pu_log(LL_WARNING, "Client write was interrupted");
                        }
                    }
                    len = sizeof(agent_wr_buf);
                }
                break;
            }
            case PS_Timeout:
//                          pu_log(LL_WARNING, "agent_thread: timeout on waiting from server queue");
                break;
            case PS_STOP:
                agent_rw_stop = 1;
                finish = 1;
                pu_log(LL_INFO, "Agent write thread received STOP event. Terminated");
                break;
            default:
                pu_log(LL_ERROR, "Agent write thread: Undefined event %d on wait!", ev);
                break;
        }

    }
    pu_log(LL_INFO, "Server's write is finished");
    pthread_exit(NULL);
}
//The only main_agent_thread data! moved out to decrease the stack size
static pthread_t agentReadThreadId;
static pthread_attr_t agentReadThreadAttr;
static pthread_t agentWriteThreadId;
static pthread_attr_t agentWriteThreadAttr;
//
static void* main_agent_thread(void *dummy_params) {
    while(!finish) {
        agent_rw_stop = 0;
        pt_tcp_rw_t rw = {-1, -1, -1};

        if(pt_tcp_server_connect(pc_getAgentPort(), &rw)) {
            pthread_attr_init(&agentReadThreadAttr);
            if (pthread_create(&agentReadThreadId, &agentReadThreadAttr, &agent_read_proc, &(rw.rd_socket))) {
                pu_log(LL_ERROR, "[agent_read_thread]: Creating write thread failed: %s", strerror(errno));
                break;
            }
            pu_log(LL_INFO, "[agent_read_thread]: started.");

            pthread_attr_init(&agentWriteThreadAttr);
            if (pthread_create(&agentWriteThreadId, &agentWriteThreadAttr, &agent_write_proc, &(rw.wr_socket))) {
                pu_log(LL_ERROR, "[server_write_thread]: Creating write thread failed: %s", strerror(errno));
                return 0;
            }
            pu_log(LL_INFO, "[agent_write_thread]: started.");

            void *ret;
            pthread_join(agentReadThreadId, &ret);
            pthread_join(agentWriteThreadId, &ret);

            pthread_attr_destroy(&agentReadThreadAttr);
            pthread_attr_destroy(&agentWriteThreadAttr);

            pt_tcp_shutdown_rw(&rw);
            pu_log(LL_WARNING, "Agent read/write threads restart");
        }
        else {
            pu_log(LL_ERROR, "Agent: connection error %d %s", errno, strerror(errno));
            sleep(1);
        }
    }
    pthread_exit(NULL);
}
//Main thread mgmt functions
///////////////////////////////////////////////////////////
//main_thread_startup/shutdown data
static pthread_t sreadThreadId;
static pthread_attr_t sreadThreadAttr;
static pthread_t swriteThreadId;
static pthread_attr_t swriteThreadAttr;
static pthread_t mainAgentThreadId;
static pthread_attr_t mainAgentThreadAttr;
//
static int main_thread_startup() {

    //Queues initiation
    pu_queues_init(Q_AMOUNT);
    to_server = pu_queue_create(pc_getQueuesRecAmt(), PS_ToServerQueue);
    from_server = pu_queue_create(pc_getQueuesRecAmt(), PS_FromServerQueue);
    to_agent = pu_queue_create(pc_getQueuesRecAmt(), PS_ToAgentQueue);
    from_agent = pu_queue_create(pc_getQueuesRecAmt(), PS_FromAgentQueue);

    if(!to_server || !from_server || !to_agent || !from_agent) return 0;

    pu_log(LL_INFO, "Proxy main thread: initiated");

//Threads start
    pthread_attr_init(&sreadThreadAttr);
    if (pthread_create(&sreadThreadId, &sreadThreadAttr, &server_read_thread, NULL)) {
        pu_log(LL_ERROR, "[server_read_thread]: Creating write thread failed: %s", strerror(errno));
        return 0;
    }
    pu_log(LL_INFO, "[server_read_thread]: started.");

    pthread_attr_init(&swriteThreadAttr);
    if (pthread_create(&swriteThreadId, &swriteThreadAttr, &server_write_thread, NULL)) {
        pu_log(LL_ERROR, "[server_write_thread]: Creating write thread failed: %s", strerror(errno));
        return 0;
    }
    pu_log(LL_INFO, "[server_write_thread]: started.");

    pthread_attr_init(&mainAgentThreadAttr);
    if (pthread_create(&mainAgentThreadId, &mainAgentThreadAttr, &main_agent_thread, NULL)) {
        pu_log(LL_ERROR, "[agent_thread]: Creating write thread failed: %s", strerror(errno));
        return 0;
    }
    pu_log(LL_INFO, "[agent_thread]: started.");

    return 1;
}
static void main_thread_shutdown() {
    void* ret;
    finish = 1;
    pu_queue_stop(to_server);
    pu_queue_stop(to_agent);
    pu_queue_stop(from_agent);
    pu_queue_stop(from_server);

    pthread_join(sreadThreadId, &ret);
    pthread_join(swriteThreadId, &ret);
    pthread_join(mainAgentThreadId, &ret);

    pthread_attr_destroy(&sreadThreadAttr);
    pthread_attr_destroy(&swriteThreadAttr);
    pthread_attr_destroy(&mainAgentThreadAttr);

//Queues destroying
    pu_queue_erase(from_server);
    pu_queue_erase(to_server);
    pu_queue_erase(from_agent);
    pu_queue_erase(to_agent);

    pu_queues_destroy();

    pu_log(LL_INFO, "Server Main is finished");
}
