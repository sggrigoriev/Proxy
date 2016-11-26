//
// Created by gsg on 03/11/16.
//
#include <pthread.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <curl/curl.h>
#include <zconf.h>

#include "pc_defaults.h"
#include "pc_settings.h"
#include "pt_tcp_utl.h"
#include "pt_http_utl.h"
#include "pu_queue.h"
#include "pu_logger.h"

////////////////////////////////////////////
// Global params to be set in common configuration
pu_queue_t* to_server;
pu_queue_t* from_server;
pu_queue_t* to_agent;
pu_queue_t* from_agent;

static volatile int finish = 0;

/** Thread attributes */
static pthread_t sreadThreadId;
static pthread_attr_t sreadThreadAttr;
static pthread_t swriteThreadId;
static pthread_attr_t swriteThreadAttr;
static pthread_t agentThreadId;
static pthread_attr_t agentThreadAttr;

//All local threads prototypes
static volatile int rt_stops = 1;
static void* server_read_thread(void* dummy_params);
static volatile int wt_stops = 1;
static void* server_write_thread(void* dummy_params);
static volatile int at_stops = 1;
static void* agent_thread(void *dummy_params);

void static stop_children();
void static main_thread_shutdown();

//Main thread mgmt functions
int static main_thread_startup() {
    if(daemon(0, 0) < 0) {
        fprintf(stderr, "Proxy main thread: Daemonization failed. Err=%d, %s\n", errno, strerror(errno));
        return 0;
    }

    if(!pc_load_config(DEFAULT_CFG_FILE_NAME)) return 0;

    pu_start_logger(pc_getLogFileName(), pc_getLogRecordsAmt(), pc_getLogVevel());

    if(!pt_http_curl_init()) return 0;

    //Queues initiation
    pu_queues_init();
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

    pthread_attr_init(&agentThreadAttr);
    if (pthread_create(&agentThreadId, &agentThreadAttr, &agent_thread, NULL)) {
        pu_log(LL_ERROR, "[agent_thread]: Creating write thread failed: %s", strerror(errno));
        return 0;
    }
    pu_log(LL_INFO, "[agent_thread]: started.");

    return 1;
}
void static main_thread_shutdown() {
    void* ret;
    stop_children();
    sleep(10);
    pthread_join(sreadThreadId, &ret);
    pthread_join(swriteThreadId, &ret);
    pthread_join(agentThreadId, &ret);

    pthread_attr_destroy(&sreadThreadAttr);
    pthread_attr_destroy(&swriteThreadAttr);
    pthread_attr_destroy(&agentThreadAttr);

//Queues destroying
    pu_queue_erase(from_server);
    pu_queue_erase(to_server);
    pu_queue_erase(from_agent);
    pu_queue_erase(to_agent);

    pu_queues_destroy();

    pu_log(LL_INFO, "Server Main is finished");

//cURL stop
    curl_global_cleanup();

//Logger stop
    pu_stop_logger();
}

char wr_src[1000];
__time_t prev_sec = 0;
__time_t to = 60;
void static stop_children() {
    int attempts = 0;

    struct timeval now;

    gettimeofday(&now, NULL);
    if(now.tv_sec > prev_sec+to) {

        finish = 1;

        while ((attempts++ < 10) && (!wt_stops || !at_stops || !rt_stops)) sleep(1);

        if (!wt_stops) pu_log(LL_ERROR, "write_to_server thread wasn't shotdown");
        if (!rt_stops) pu_log(LL_ERROR, "read_from_server thread wasn't shotdown");
        if (!at_stops) pu_log(LL_ERROR, "agent thread wasn't shotdown");
    }
}
///////////////////////////////////////////////////////////////////////////////
//
//
static pu_queue_msg_t mt_msg[PROXY_MAX_MSG_LEN];
int pt_main_thread_start() { //Starts and detach the main thread.
    pu_queue_events_set_t events;
    pu_clear_queue_events(&events);

    if(!main_thread_startup()) finish = 1;

    pu_add_queue_event(&events, PS_FromAgentQueue);
    pu_add_queue_event(&events, PS_FromServerQueue);

    while(!finish) {
        unsigned int len = sizeof(mt_msg);
        pu_queue_event_t ev;

        switch (ev=pu_wait_for_queues(&events, DEFAULT_MAIN_THREAD_TO_SEC)) {
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
                    pu_log(LL_DEBUG, "maim_thread: msg %s was sent to server_write", mt_msg);
                    len = sizeof(mt_msg);
                }
                break;
            default:
                pu_log(LL_ERROR, "main_thread: Undefined event %d on wait (from agent / from server)!", ev);
                break;
        }

    }
    main_thread_shutdown();
    return 0;
}

static void* server_read_thread(void* dummy_params) {
    char *buf = NULL;
    rt_stops = 0;

    if (!pt_http_read_init()) {
        pu_log(LL_ERROR, "server_read_thread: Cloud info read initiation failed. Stop.");
        pthread_exit(NULL);
    }

    rt_stops = 0;
//Main read loop
    while(!finish) {
        int rc = 0;
        while(rc <= 0) {
            rc = pt_http_read(&buf);
        }
        pu_queue_push(from_server, buf, rc+1);  //NB! Possibly needs to split info to 0-terminated strings!
        pu_log(LL_INFO, "Received from cloud: %s", buf);
    }
    pt_http_read_destroy();
    rt_stops = 1;
    pthread_exit(NULL);
}
static pu_queue_msg_t wt_msg[PROXY_MAX_MSG_LEN];
static void* server_write_thread(void* dummy_params) {
    pu_queue_events_set_t events;
    pu_clear_queue_events(&events);


    if(!pt_http_write_init()) {
        pu_log(LL_ERROR, "server_write_thread: Cloud info write initiation failed. Stop.");
        pthread_exit(NULL);
    }

    pu_add_queue_event(&events, PS_ToServerQueue);

    wt_stops = 0;
    //Main write loop
    while(!finish) {

        if(pu_wait_for_queues(&events, DEFAULT_SERVER_WRITE_THREAD_TO_SEC)) {
             unsigned int len = sizeof(wt_msg);
             while(pu_queue_pop(to_server, wt_msg, &len)) {
                 pu_log(LL_DEBUG, "Got from from main by server_write_thread: %s", wt_msg);

                 int rc = 0;
                 char *resp;
                 unsigned int resp_len = 0;
                 while (rc <= 0) {
                     rc = pt_http_write(wt_msg, len, &resp, &resp_len);
                 }
                 pu_log(LL_INFO, "Sent to cloud: %s", wt_msg);
                 if (resp_len > 0) {
                     pu_log(LL_INFO, "Answer from cloud: %s", wt_msg);
                     pu_queue_push(from_server, resp, resp_len);
                 }
                 len = sizeof(wt_msg);
             }
        }
        else {
            pu_log(LL_WARNING, "server_write_thread: timeout on waiting to server queue");
        }
    }
    pt_http_write_destroy();
    wt_stops = 1;
    pthread_exit(NULL);
}
////////////////////////////////////
//The only agent_thread buffers! moved out to decrease the stack size
static char abuf[DEFAULT_TCP_ASSEMBLING_BUF_SIZE];
static char rbuf[PROXY_MAX_MSG_LEN];
static void* agent_thread(void *dummy_params) {
    int socket = -1;
    int reconnect = 0;
    pu_queue_events_set_t events;
    pu_clear_queue_events(&events);

    pu_add_queue_event(&events, PS_ToAgentQueue);

    at_stops = 0;
    while(socket = pt_tcp_server_connect(pc_getAgentPort(), socket, reconnect), (socket > 0) && (!finish)) {
        pu_log(LL_INFO, "SERVER: TCP communications: connected");
        reconnect = 0;

        while(!reconnect) {
            int ret;
            pt_tcp_assembling_buf_t as_buf = {abuf, sizeof(abuf), 0, rbuf, sizeof(rbuf)};

            switch(pt_tcp_selector(socket)) {
                case CU_READ: {
                    // read
                    while(1) {
                        if (ret = pt_tcp_read(socket, rbuf, sizeof(rbuf)), ret > 0) { //read smth
                            const char * msg = pt_tcp_assemble(rbuf, ret, &as_buf);
                            if(msg) {
                                pu_queue_push(to_server, msg, strlen(msg)+1);
                                pu_log(LL_INFO, "agent_thread got from AGENT: %s", msg);
                                break;
                            }
                        } else if (ret < 0) { // reconnect
                            pu_log(LL_WARNING, "agent_thread reconnect: read returns 0");
                            reconnect = 1;
                            break;
                        } else if (ret == 0) {
//                            pu_log(LL_DEBUG, "agent_thread from AGENT: TIMEOUT");
                            break;
                        }
                    }
                    break;
                }
                case CU_WRITE: {
                    //write
                    if(pu_wait_for_queues(&events, DEFAULT_AGENT_THREAD_TO_SEC)) {
                        unsigned int len = sizeof(rbuf);
                        while(pu_queue_pop(to_agent, rbuf, &len)) {
                            if(ret = pt_tcp_write(socket, rbuf, strlen(rbuf)+1), ret > 0) { //write smth
                                pu_log(LL_DEBUG, "agent_thread: sent to AGENT %d bytes: %s", ret, rbuf);
                                break;
                            }
                            else if(ret <= 0 ) { //reconnect
                                pu_log(LL_WARNING, "agent_thread reconnect: write returns 0");
                                reconnect = 1;
                                break;
                            }
                            len = sizeof(rbuf);
                        }
                    }
                    else {
//                        pu_log(LL_WARNING, "agent_thread: timeout on waiting from server queue");
                    }
                    break;
                }
                case CU_TIMEOUT:
                    break;
                case CU_ERROR:
                    reconnect = 1;
                    break;
            }
        }
    }
    pu_log(LL_INFO, "Server Thread is finished");
    pt_tcp_shutdown(socket);
    at_stops = 1;
    pthread_exit(NULL);
}