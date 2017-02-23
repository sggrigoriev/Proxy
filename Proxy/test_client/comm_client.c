//
// Created by gsg on 03/11/16.
//
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "lib_tcp.h"
#include "lib_timer.h"
#include "pr_commands.h"
#include "pc_defaults.h"
#include "pc_settings.h"
//

//{"version": "2", "proxyId": "PROXY_ID", "sequenceNumber": "13117",
//  "measures": [{"deviceId": "DEVICE_ID_NO_TIMESTAMP",
//      "params": [{"name": "desc","value": "Send a measurement from an existing device, with no timestamp"},{"name": "power","value": "100"}]
//  ]
//}
#define SEND_TO_SEC 3000
static lib_timer_clock_t send_clock = {0};
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
    char* ret = NULL;

    if(lib_timer_alarm(send_clock)) {
        ret = wr_src;
        lib_timer_init(&send_clock, SEND_TO_SEC);
    }
    return ret;
}
#ifndef PROXY_SEPARATE_RUN
static lib_timer_clock_t wd_clock = {0};
static char wd_alert[100];
// make Watchdog for the WUD
static void make_wd() {
    char di[LIB_HTTP_DEVICE_ID_SIZE];
    pc_getProxyDeviceID(di, sizeof(di));
    pr_make_wd_alert4WUD(wd_alert, sizeof(wd_alert), "Agent", di);
}
static const char* wud_source() {
    if(!lib_timer_alarm(wd_clock)) return NULL;
    lib_timer_init(&wd_clock, pc_getProxyWDTO());
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

static void* wud_proc(void* socket) {
    int wud_socket = *(int *)socket;

    pu_log(LL_DEBUG, "WUD socket = %d", wud_socket);

//Main loop
    while(!rw_stop) {
        //Get smth to write
        const char* info = wud_source(); //Return NULL if no need to send WD alert or WD alert
        sleep(1);
        if(!info) continue;
//Start write operation
        ssize_t ret;
        while(ret = lib_tcp_write(wud_socket, info, strlen(info)+1, 1), !ret&&!rw_stop);  //run until the timeout
        if(rw_stop) continue; // goto reconnect
        if(ret < 0) { //op start failed
            pu_log(LL_ERROR, "wud thread: Write op start failed %d %s. Reconnect", errno, strerror(errno));
            rw_stop = 1;
        }
        pu_log(LL_DEBUG, "wud thread: Sent to WUD: %s", info);
    }
    lib_tcp_client_close(wud_socket);
    pu_log(LL_INFO, "wud thread is finished");
    pthread_exit(NULL);
}
#endif
////////////////////////////////////////////////////
static char out_buf[500];


static void* read_proc(void* socket) {
    int read_socket = *(int *)socket;

    lib_tcp_conn_t* all_conns = lib_tcp_init_conns(1, 500, 10000);
    if(!all_conns) {
        pu_log(LL_ERROR, "%s: memory allocation error.", "read_proc");
        rw_stop = 1;
        goto allez;      //Allez kaput
    }
    if(!lib_tcp_add_new_conn(read_socket, all_conns)) {
        pu_log(LL_ERROR, "%s: new incoming connection exeeds max amount. Aborted", "read_proc");
        rw_stop = 1;
        goto allez;
    }

    while(!rw_stop) {
        lib_tcp_rd_t *conn = lib_tcp_read(all_conns, 1); //connection removed inside
        if (!conn) {
            pu_log(LL_ERROR, "%s: read error. Reconnect. %d %s", "read_proc", errno, strerror(errno));
            rw_stop = 1;
            break;
        }
        if (conn == LIB_TCP_READ_TIMEOUT) {
            continue;   //timeout
        }
        if (conn == LIB_TCP_READ_MSG_TOO_LONG) {
            pu_log(LL_ERROR, "%s: incoming message too long and can't be assempled. Ignored", "read_proc");
            continue;
        }
        if (conn == LIB_TCP_READ_NO_READY_CONNS) {
            pu_log(LL_ERROR, "%s: internal error - no ready sockets. Ignored", "read_proc");
            break;
        }
        if (conn == LIB_TCP_READ_EOF) {
            pu_log(LL_ERROR, "%s: remote connection closed. Restart", "read_proc");
            rw_stop = 1;
            break;
        }
        while (lib_tcp_assemble(conn, out_buf, sizeof(out_buf))) {     //Reag all fully incoming messages
            pu_log(LL_INFO, "%s: got from Proxy: %s", "read_proc", out_buf);
        }
    }
    allez:
    lib_tcp_destroy_conns(all_conns);
    pu_log(LL_INFO, "Client's read is finished");
    pthread_exit(NULL);
}
////////////////////////////////////////////////////////////////////////////////////////////////////////

static void* write_proc(void* socket) {
    int write_socket = *(int *)socket;
    lib_timer_init(&send_clock, SEND_TO_SEC);
//Main loop
    while(!rw_stop) {
        //Get smth to write
         const char* info = write_source(); //should be wait for info from queue(s)
        sleep(1);
        if(!info) continue;

        //Prepare write operation
        ssize_t ret;
        while(ret = lib_tcp_write(write_socket, info, strlen(info)+1, 1), !ret&&!rw_stop);  //run until the timeout
        if(rw_stop) continue; // goto reconnect
        if(ret < 0) { //op start failed
            pu_log(LL_ERROR, "write_proc: Write op start failed %d %s. Reconnect", errno, strerror(errno));
            rw_stop = 1;
        }
        pu_log(LL_DEBUG, "write_proc: sent to Proxy: %s", info);
    }
    lib_tcp_client_close(write_socket);
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

        int wud = -1;
        pu_log(LL_INFO, "pc_getAgentPort() = %d, pc_getWUDPort() = %d", pc_getAgentPort(), pc_getWUDPort());
        lib_timer_init(&wd_clock, pc_getProxyWDTO());

        while(wud = lib_tcp_get_client_socket(pc_getWUDPort(), 1), wud <= 0) {
            pu_log(LL_ERROR, "Client: connection error to WUD %d %s", errno, strerror(errno));
            sleep(1);
        }

        pthread_attr_init(&wudThreadAttr);
        if (pthread_create(&wudThreadId, &wudThreadAttr, &wud_proc, &wud)) {
            pu_log(LL_ERROR, "Client: Creating watchdog thread failed: %s", strerror(errno));
            break;
        }
        pu_log(LL_INFO, "Client: watchdog started. Port = %d, Socket = %d", pc_getWUDPort(), wud);
#endif

        int read_socket = -1;
        int write_socket = -1;

        while(read_socket = lib_tcp_get_client_socket(pc_getAgentPort(), 1), read_socket <= 0) {
            pu_log(LL_ERROR, "Client: connection error to Proxy %d %s", errno, strerror(errno));
            sleep(1);
        }
        write_socket = dup(read_socket);
//Connected to proxy
        pthread_attr_init(&readThreadAttr);
        if (pthread_create(&readThreadId, &readThreadAttr, &read_proc, &read_socket)) {
            pu_log(LL_ERROR, "Client: Creating read thread failed: %s", strerror(errno));
            break;
        }
        pu_log(LL_INFO, "Client: read started.");

        pthread_attr_init(&writeThreadAttr);
        if (pthread_create(&writeThreadId, &writeThreadAttr, &write_proc, &write_socket)) {
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

    pu_start_logger("./AGENT_LOG", 5000, LL_DEBUG);

    main_client_proc(NULL);

    pu_log(LL_INFO, "Client Main is finished");
    pu_stop_logger();
    return 1;
}