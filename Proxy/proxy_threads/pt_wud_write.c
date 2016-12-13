//
// Created by gsg on 12/12/16.
//
#include <aio.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>

#include "pc_defaults.h"
#include "pu_logger.h"
#include "pt_queues.h"
#include "pt_main_agent.h"
#include "pt_wud_write.h"


#define PT_THREAD_NAME "WUD_WRITE"

////////////////////////////
static pthread_t id;
static pthread_attr_t attr;
static volatile int stop;   //one stop for write and read agent threads

static struct aiocb cb;
static const struct aiocb* cb_list[1];
static char out_buf[PROXY_MAX_MSG_LEN*2];
static struct timespec wud_wr_timeout = {1,0};

static int write_socket;
static pu_queue_t* to_wud;

static void* wud_write(void* params);

int start_wud_write() {
    if(pthread_attr_init(&attr)) return 0;
    if(pthread_create(&id, &attr, &wud_write, NULL)) return 0;
    return 1;
}

void stop_wud_write() {
    void *ret;
    set_stop_wud_write();
    
    pthread_join(id, &ret);
    pthread_attr_destroy(&attr);
}

void set_stop_wud_write() {
    stop = 1;
}
static void* wud_write(void* params) {
    return NULL;
}



