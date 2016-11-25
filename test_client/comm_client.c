//
// Created by gsg on 03/11/16.
//
#include <pthread.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <zconf.h>
#include <sys/time.h>

#include "pc_defaults.h"
#include "pc_settings.h"
#include "pt_tcp_utl.h"
#include "pu_logger.h"
//{"version": "2", "proxyId": "PROXY_ID", "sequenceNumber": "13117",
//  "measures": [{"deviceId": "DEVICE_ID_NO_TIMESTAMP",
//      "params": [{"name": "desc","value": "Send a measurement from an existing device, with no timestamp"},{"name": "power","value": "100"}]
//  ]
//}
char wr_src[1000];
__time_t prev_sec = 0;
__time_t to = 10;

const char* write_source() {
    snprintf ( wr_src, sizeof(wr_src), "%s",
        "{\"version\": 2, \"proxyId\": \"aioxGW-GSGTest_deviceid\", \"sequenceNumber\": \"13117\","
            "\"measures\": [{\"deviceId\": \"DEVICE_ID_NO_TIMESTAMP\","
                "\"params\": [{\"name\": \"desc\",\"value\": \"Send a measurement from an existing device, with no timestamp\"},{\"name\": \"power\",\"value\": \"100\"}]"
            "}]"
        "}"
    );
    struct timeval now;

    gettimeofday(&now, NULL);
    if(now.tv_sec > prev_sec+to) {
        prev_sec = now.tv_sec;
        return wr_src;
    }

    return NULL;
}

static int finish = 0;
static int stop = 0;

void* client_thread(void* dummy_param) {

    int socket = -1;
    while(socket = pt_tcp_client_connect(pc_getAgentPort(), socket), (socket > 0) && (!finish)) {

        pu_log(LL_INFO, "CLIENT: connected");
        stop = 0;
        while(!stop) {
            char abuf[DEFAULT_TCP_ASSEMBLING_BUF_SIZE];
            char rbuf[PROXY_MAX_MSG_LEN];

            int ret;

            pt_tcp_assembling_buf_t as_buf = {abuf, sizeof(abuf), 0, rbuf, sizeof(rbuf)};

            switch(pt_tcp_selector(socket)) {
                case CU_READ: {
// read
                    while(1) {
                        if (ret = pt_tcp_read(socket, rbuf, sizeof(rbuf)), ret > 0) { //read smth
                            const char * msg = pt_tcp_assemble(rbuf, ret, &as_buf);
                            if(msg) {
                                pu_log(LL_INFO, "CLIENT READ: %s", msg);
                                break;
                            }
                        } else if (ret < 0) { // reconnect
                            pu_log(LL_WARNING, "CLIENT RECONNECT: read returns 0");
                            stop = 1;
                            break;
                        } else if (ret == 0) { //timeout
                            pu_log(LL_DEBUG, "CLIENT READ: TIMEOUT");
                            break;
                        }
                    }
                    break;
                }
                case CU_WRITE: {
//write
                    const char * wr = write_source();
                    if(!wr) break;
                    if(ret = pt_tcp_write(socket, wr, strlen(wr)+1), ret > 0) { //write smth
                        pu_log(LL_DEBUG, "CLIENT WRITE: %d bytes: %s", ret, wr);
                        break;
                    }
                    else if(ret <= 0 ) { //reconnect
                        pu_log(LL_WARNING, "CLIENT RECONNECT: write returns 0");
                        stop= 1;
                        continue;
                    }
                    break;
                }
                case CU_TIMEOUT:
                    pu_log(LL_DEBUG, "CLIENT SELECT: timeout");
                    continue;
                case CU_ERROR:
                    stop = 1;
                    continue;
            }
        }
    }
    pu_log(LL_INFO, "Client Thread is finished");
    pt_tcp_shutdown(socket);
    return NULL;
};

/** Thread attributes */
static pthread_t clThreadId;
static pthread_attr_t clThreadAttr;

int main() {

    if(!pc_load_config(DEFAULT_CFG_FILE_NAME)) {
        fprintf(stderr, "Client thread exited\n");
        return -1;
    }

    pu_start_logger(/*"./COMM_LOG"*/NULL, 5000, LL_INFO );

    pthread_attr_init(&clThreadAttr);

    if (pthread_create(&clThreadId, &clThreadAttr, &client_thread, NULL)) {
        pu_log(LL_ERROR, "[CLIENT]: Creating write thread failed: %s", strerror(errno));
        return -1;
    }

    getchar();
    finish = 1;
    void* ret;
    pthread_join(clThreadId, &ret);

    pu_log(LL_INFO, "Client Main is finished");
    pu_stop_logger();
    return 1;
}