//
// Created by gsg on 03/11/16.
//
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <aio.h>
#include <errno.h>
#include <pt_tcp_utl.h>

#include "pc_defaults.h"
#include "pc_settings.h"

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
    sleep(5);
    return wr_src;
}
static int finish = 0;
////////////////////////////////////////////////////
static volatile int rw_stop = 0;
////////////////////////////////////////////////////
static struct aiocb rd_cb;
static const struct aiocb* rd_cb_list[1];
static char out_buf[500];
static char rbuf[1000];
static char ass_buf[10000];
static struct timespec rd_timeout = {10,0};

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
        int ret = aio_suspend(rd_cb_list, 1, &rd_timeout); //wait until the read
        if(ret == 0) {                                      //Read succeed
            ssize_t bytes_read = aio_return(&rd_cb);
            if(bytes_read > 0) {
                pt_tcp_get(rbuf, bytes_read, &as_buf);
                while(pt_tcp_assemble(out_buf, sizeof(out_buf), &as_buf)) {     //Reag all fully incoming messages
                    pu_log(LL_INFO, "From server: %s", out_buf);
                }
            }
            else {  //error in read
                pu_log(LL_ERROR, "Client. Read op failed %d %s. Reconnect", errno, strerror(errno));
                rw_stop = 1;
            }
            if (aio_read(&rd_cb) < 0) {                                         //Run read operation again
                pu_log(LL_ERROR, "read proc: aio_read: start operation failed %d %s", errno, strerror(errno));
                rw_stop = 1;
            }
        }
        else if (ret == EAGAIN) {   //Timeout
            pu_log(LL_DEBUG, "Client read: timeout");
        }
        else {
//            pu_log(LL_WARNING, "Client read was interrupted. ");
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
        int ret = aio_write(&wr_cb);
        if(ret != 0) { //op start failed
            pu_log(LL_ERROR, "Client. Write op start failed %d %s. Reconnect", errno, strerror(errno));
            rw_stop = 1;
        }
        else {
            int wait_ret = aio_suspend(wr_cb_list, 1, &wr_timeout); //wait until the write
            if(wait_ret == 0) {                                         //error in write
                ssize_t bytes_written = aio_return(&wr_cb);
                if(bytes_written <= 0) {
                    pu_log(LL_ERROR, "Client. Write op failed %d %s. Reconnect", errno, strerror(errno));
                    rw_stop = 1;
                }
            }
            else if (wait_ret == EAGAIN) {   //Timeout
                pu_log(LL_DEBUG, "Client write: timeout");
            }
            else {
//                pu_log(LL_WARNING, "Client write was interrupted");
            }
        }
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
        pt_tcp_rw_t rw = {-1, -1, -1};

        if(pt_tcp_client_connect(pc_getAgentPort(), &rw)) {
            pthread_attr_init(&readThreadAttr);
            if (pthread_create(&readThreadId, &readThreadAttr, &read_proc, &(rw.rd_socket))) {
                pu_log(LL_ERROR, "[client_read_thread]: Creating write thread failed: %s", strerror(errno));
                break;
            }
            pu_log(LL_INFO, "[client_read_thread]: started.");

            pthread_attr_init(&writeThreadAttr);
            if (pthread_create(&writeThreadId, &writeThreadAttr, &write_proc, &(rw.wr_socket))) {
                pu_log(LL_ERROR, "[client_write_thread]: Creating write thread failed: %s", strerror(errno));
                return 0;
            }
            pu_log(LL_INFO, "[client_write_thread]: started.");

            void *ret;
            pthread_join(readThreadId, &ret);
            pthread_join(writeThreadId, &ret);

            pthread_attr_destroy(&readThreadAttr);
            pthread_attr_destroy(&writeThreadAttr);

            pt_tcp_shutdown_rw(&rw);
            pu_log(LL_WARNING, "Client read/write threads restart");
        }
        else {
            pu_log(LL_ERROR, "Client: connection error %d %s", errno, strerror(errno));
            sleep(1);
        }
    }
    pthread_exit(NULL);
}
////////////////////////////////////////////////////////////////////////////////////////////////////////
int main(int argv, char* argc[]) {
    pu_start_logger(/*"./COMM_LOG"*/NULL, 5000, LL_INFO );

    main_client_proc(NULL);

    pu_log(LL_INFO, "Client Main is finished");
    pu_stop_logger();
    return 1;
}