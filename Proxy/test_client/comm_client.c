//
// Created by gsg on 03/11/16.
//
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <aio.h>
#include <errno.h>
#include <pt_tcp_utl.h>
#include <pf_proxy_watchdog.h>
#include <pr_commands.h>

#include "pc_defaults.h"
#include "pc_settings.h"
//

//{"version": "2", "proxyId": "PROXY_ID", "sequenceNumber": "13117",
//  "measures": [{"deviceId": "DEVICE_ID_NO_TIMESTAMP",
//      "params": [{"name": "desc","value": "Send a measurement from an existing device, with no timestamp"},{"name": "power","value": "100"}]
//  ]
//}
static char wr_src[1000];
const char* write_source() {
    snprintf ( wr_src, sizeof(wr_src), "%s",
"{"
    "\"measures\": "
    "["
        "{"
            "\"deviceId\": \"DEVICE_ID_NO_TIMESTAMP\","
            "\"params\": "
            "["
                "{"
                    "\"name\": \"desc\","
                    "\"value\": \"Send a measurement from an existing device, with no timestamp\""
                "},"
                "{"
                    "\"name\": \"power\","
                    "\"value\": \"100\""
                "}"
            "]"
        "}"
    "]"
"}"
);
    sleep(30);
    return wr_src;
}
#ifndef PROXY_SEPARATE_RUN
static pf_clock_t wd_clock = {0};
static char wd_alert[100];
// make Watchdog for the WUD
static void make_wd() {
    char *agent_info;
    pr_message_t msg;

    msg.watchdog_alert.message_type = PR_WATCHDOG_ALERT;
    msg.watchdog_alert.process_name = strdup("Agent");

    pr_make_message(&agent_info, msg);
    strncpy(wd_alert, agent_info, sizeof(wd_alert)-1);
    pr_erase_msg(msg);
    free(agent_info);
}
static const char* wud_source() {
    sleep(1);
    if(!pf_wd_time_to_send(&wd_clock)) return NULL;
    make_wd();
    return wd_alert;
}
#endif
static volatile int finish = 0;
////////////////////////////////////////////////////
static volatile int rw_stop = 0;
////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef PROXY_SEPARATE_RUN
static pthread_t wudThreadId;
static pthread_attr_t wudThreadAttr;
static char wudbuf[1000];

static void* wd_write(void* sock) {
    int client_socket;
    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    int32_t on = 1;
    setsockopt(client_socket, SOL_SOCKET, SO_REUSEADDR, &on, sizeof (on));
    struct sockaddr_in addr_struct;
    memset(&addr_struct, 0, sizeof(addr_struct));
    addr_struct.sin_family = AF_INET;
    addr_struct.sin_addr.s_addr = INADDR_ANY;
    addr_struct.sin_port = htons(8888);

    if(connect(client_socket, (struct sockaddr *)&addr_struct, sizeof(addr_struct)) < 0 ) {
        pu_log(LL_ERROR, "CU_CLEINT_CONNECT: error connection %d, %s", errno, strerror(errno));
        exit(1);
    }

    while(!rw_stop) {
        //Get smth to write
        const char* info = wud_source(); //Return NULL if no need to send WD alert or WD alert
        if(!info) continue;

        ssize_t ret = write(client_socket, info, strlen(info)+1);

        if(ret <= 0) { ////error in write
            pu_log(LL_ERROR, "wud thread: Write op failed %d %s. Reconnect", errno, strerror(errno));
            rw_stop = 1;
        }
        else {
            pu_log(LL_DEBUG, "wud thread: Sent to WUD: %s", info);
        }
    }
    pu_log(LL_INFO, "wud thread is finished");
    pthread_exit(NULL);
}
static void* wud_proc(void* socket) {
    struct aiocb wud_cb;
    const struct aiocb* wud_cb_list[1];
    struct timespec wud_timeout = {1,0};

    int wud_socket = *(int *)socket;
//Init section
    memset(&wud_cb, 0, sizeof(struct aiocb));
    wud_cb.aio_buf = wudbuf;
    wud_cb.aio_fildes = wud_socket;
    wud_cb.aio_nbytes = sizeof(wudbuf)-1;
    wud_cb.aio_offset = 0;

    memset(&wud_cb_list, 0, sizeof(wud_cb_list));
    wud_cb_list[0] = &wud_cb;

    pu_log(LL_DEBUG, "WUD socket = %d", wud_socket);

//Main loop
    while(!rw_stop) {
        //Get smth to write
        const char* info = wud_source(); //Return NULL if no need to send WD alert or WD alert
        if(!info) continue;
//Prepare write operation
        strcpy(wudbuf, info);
        wud_cb.aio_nbytes = strlen(wudbuf) + 1;
//Start write operation
        if(aio_write(&wud_cb)) { //op start failed
            pu_log(LL_ERROR, "wud thread: Write op start failed %d %s. Reconnect", errno, strerror(errno));
            rw_stop = 1;
        }
        while(aio_suspend(wud_cb_list, 1, &wud_timeout) && !rw_stop);            //wait until the write

        if(aio_return(&wud_cb) <= 0) { ////error in write
            pu_log(LL_ERROR, "wud thread: Write op failed %d %s. Reconnect", errno, strerror(errno));
            rw_stop = 1;
        }
        else {
            pu_log(LL_DEBUG, "wud thread: Sent to WUD: %s", wudbuf);
        }
    }
    pu_log(LL_INFO, "wud thread is finished");
    pthread_exit(NULL);
}
#endif
////////////////////////////////////////////////////
static struct aiocb rd_cb;
static const struct aiocb* rd_cb_list[1];
static char out_buf[500];
static char rbuf[1000];
static char ass_buf[10000] = {0};
static struct timespec rd_timeout = {1,0};

static void* read_proc(void* socket) {
    int read_socket = *(int *)socket;

//Init section
    pt_tcp_assembling_buf_t as_buf = {ass_buf, sizeof(ass_buf), 0};

    memset(&rd_cb, 0, sizeof(struct aiocb));
    rd_cb.aio_buf = rbuf;
    rd_cb.aio_fildes = read_socket;
    rd_cb.aio_nbytes = sizeof(rbuf)-1;
    rd_cb.aio_offset = 0;

    memset(&rd_cb_list, 0, sizeof(rd_cb_list));
    rd_cb_list[0] = &rd_cb;

    if (aio_read(&rd_cb) < 0) {                 //start read operation
        pu_log(LL_ERROR, "read proc: aio_read: start operation failed %d %s", errno, strerror(errno));
        rw_stop = 1;
    }
    while(!rw_stop) {
        ssize_t bytes_read;
        if(aio_suspend(rd_cb_list, 1, &rd_timeout)) {
            if (errno == EAGAIN) {   //Timeout
                pu_log(LL_DEBUG, "Client read: timeout");
            }
            else {
//            pu_log(LL_WARNING, "Client read was interrupted. ");
            }
        }
        else if(bytes_read = aio_return(&rd_cb), bytes_read <= 0) {
            pu_log(LL_ERROR, "Client. Read op failed %d %s. Reconnect", errno, strerror(errno));
            rw_stop = 1;
        }
        else if(pt_tcp_get(rbuf, bytes_read, &as_buf)) {
            while(pt_tcp_assemble(out_buf, sizeof(out_buf), &as_buf)) {     //Reag all fully incoming messages
                pu_log(LL_INFO, "Client. From server: %s", out_buf);
            }
        }
        else {
            pu_log(LL_ERROR, "%Client. incoming mesage too large: %lu vs %lu. Ignored", bytes_read, sizeof(ass_buf));
        }
        if (aio_read(&rd_cb) < 0) {                                         //Run read operation again
           pu_log(LL_ERROR, "read proc: aio_read: start operation failed %d %s", errno, strerror(errno));
           rw_stop = 1;
        }
    }
    pu_log(LL_INFO, "Client's read is finished");
    pthread_exit(NULL);
}
////////////////////////////////////////////////////////////////////////////////////////////////////////
static struct aiocb wr_cb;
static const struct aiocb* wr_cb_list[1];
static char wrbuf[1000];
static struct timespec wr_timeout = {1,0};

static void* write_proc(void* socket) {
    int write_socket = *(int *)socket;
//Init section
    memset(&wr_cb, 0, sizeof(struct aiocb));
    wr_cb.aio_buf = wrbuf;
    wr_cb.aio_fildes = write_socket;
    wr_cb.aio_nbytes = sizeof(wrbuf)-1;
    wr_cb.aio_offset = 0;

    memset(&wr_cb_list, 0, sizeof(wr_cb_list));
    wr_cb_list[0] = &wr_cb;

//Main loop
    while(!rw_stop) {
        //Get smth to write
        const char* info = write_source(); //should be wait for info from queue(s)

        //Prepare write operation
        strcpy(wrbuf, info);
        wr_cb.aio_nbytes = strlen(wrbuf) + 1;
        //Start write operation
        if(aio_write(&wr_cb)) { //op start failed
            pu_log(LL_ERROR, "Client. Write op start failed %d %s. Reconnect", errno, strerror(errno));
            rw_stop = 1;
        }

        while(aio_suspend(wr_cb_list, 1, &wr_timeout) && !rw_stop);

        if(aio_return(&wr_cb) <= 0) {
            pu_log(LL_ERROR, "Client. Write op failed %d %s. Reconnect", errno, strerror(errno));
            rw_stop = 1;
            continue;
        }
//        pu_log(LL_DEBUG, "Client: sent to Proxy: %s", wrbuf);
    }
    pu_log(LL_INFO, "Client's write is finished");
    pthread_exit(NULL);
}
////////////////////////////////////////////////////////////////////////////////////////////////////////
static pthread_t readThreadId;
static pthread_attr_t readThreadAttr;
static pthread_t writeThreadId;
static pthread_attr_t writeThreadAttr;

static void* main_client_proc(void* dummy) {

    while(!finish) {
        rw_stop = 0;

#ifndef PROXY_SEPARATE_RUN

        pt_tcp_rw_t wud = {-1, -1, -1};

        while(!pt_tcp_client_connect(pc_getWUDPort(), &wud)) {
            pu_log(LL_ERROR, "Client: connection error to WUD %d %s", errno, strerror(errno));
            sleep(1);
        }

        pthread_attr_init(&wudThreadAttr);
        if (pthread_create(&wudThreadId, &wudThreadAttr, &wud_proc, &(wud.wr_socket))) {
            pu_log(LL_ERROR, "Client: Creating watchdog thread failed: %s", strerror(errno));
            break;
        }
        pu_log(LL_INFO, "Client: watchdog started. Port = %d, Socket = %d", pc_getWUDPort(), wud.wr_socket);
#endif

        pt_tcp_rw_t rw = {-1, -1, -1};

        while(!pt_tcp_client_connect(pc_getAgentPort(), &rw)) {
            pu_log(LL_ERROR, "Client: connection error %d %s", errno, strerror(errno));
            sleep(1);
        }
//Connected to proxy
        pthread_attr_init(&readThreadAttr);
        if (pthread_create(&readThreadId, &readThreadAttr, &read_proc, &(rw.rd_socket))) {
            pu_log(LL_ERROR, "Client: Creating write thread failed: %s", strerror(errno));
            break;
        }
        pu_log(LL_INFO, "Client: read started.");

        pthread_attr_init(&writeThreadAttr);
        if (pthread_create(&writeThreadId, &writeThreadAttr, &write_proc, &(rw.wr_socket))) {
            pu_log(LL_ERROR, "Client: write Creating write thread failed: %s", strerror(errno));
                break;
        }
        pu_log(LL_INFO, "Client: write started.");

        void *ret;

        pthread_join(readThreadId, &ret);
        pthread_join(writeThreadId, &ret);

#ifndef PROXY_SEPARATE_RUN
        pthread_join(wudThreadId, &ret);
#endif

        pthread_attr_destroy(&readThreadAttr);
        pthread_attr_destroy(&writeThreadAttr);

#ifndef PROXY_SEPARATE_RUN
        pthread_attr_destroy(&wudThreadAttr);
#endif

        pt_tcp_shutdown_rw(&rw);

#ifndef PROXY_SEPARATE_RUN
        pt_tcp_shutdown_rw(&wud);
#endif
        pu_log(LL_WARNING, "Client read/write/watchdog threads restart");
    }
    pthread_exit(NULL);
}
////////////////////////////////////////////////////////////////////////////////////////////////////////
int main(int argv, char* argc[]) {
    if(!pc_load_config(DEFAULT_CFG_FILE_NAME)) {
        fprintf(stderr, "Client: error config loading %s: %d %s", DEFAULT_CFG_FILE_NAME, errno, strerror(errno));
        exit(1);
    }

    pu_start_logger("./AGENT_LOG", 5000, LL_INFO);

    main_client_proc(NULL);

    pu_log(LL_INFO, "Client Main is finished");
    pu_stop_logger();
    return 1;
}