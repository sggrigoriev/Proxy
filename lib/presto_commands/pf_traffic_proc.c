/*
 *  Copyright 2017 People Power Company
 *
 *  This code was developed with funding from People Power Company
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *

    Created by gsg on 13/12/16.
*/

#include <stdio.h>
#include <string.h>
#include <pthread.h>

#include "lib_http.h"

#include "pf_traffic_proc.h"
#include "pu_logger.h"

/************************************************************************************************

    Local data
*/
static pthread_mutex_t rd_mutex = PTHREAD_MUTEX_INITIALIZER;  /* protects counter initializer */

static unsigned long seq_number = 0UL;
/*************************************
 * Increments the sequential number. Thread protected
 * @return the number incremented
 */
static unsigned long inc_seq_number() {
    unsigned long ret;
    pthread_mutex_lock(&rd_mutex);
    ret = ++seq_number;
    pthread_mutex_unlock(&rd_mutex);
    return ret;
}

/*
 *  {JSON} -> {head_list, JSON}
 */
size_t pf_add_proxy_head(char* msg, size_t msg_size, const char* device_id) {
    unsigned long number = inc_seq_number();
    char buf[LIB_HTTP_MAX_MSG_SIZE*2];

    memset(buf,0,sizeof(buf));
    pu_log(LL_DEBUG,"%s: ===== 1 got message: '%s'",__FUNCTION__,msg);
    char *p = strchr(msg,'{');
    if (p) { /* message contains something starting with '{' */
        p++;
        sprintf(buf, "{\"proxyId\": \"%s\", \"sequenceNumber\": \"%lu\", %s\0", device_id, number, p);
    }
    else  /* empty message */
        sprintf(msg, "{\"proxyId\": \"%s\", \"sequenceNumber\": \"%lu\"}\0", device_id, number);

    strcpy(msg, buf);
    msg[msg_size-1] = '\0';
    pu_log(LL_ERROR,"%s:===== 5 resulting message: '%s'",__FUNCTION__,msg);
    return strlen(msg);
}

size_t depr_pf_add_proxy_head(char* msg, size_t msg_size, const char* device_id) {
    unsigned long number = inc_seq_number();

    pu_log(LL_DEBUG,"%s: ===== 1 got message: '%s'",__FUNCTION__,msg);
    if(!strlen(msg)) {
        pu_log(LL_ERROR,"%s: =====  2 ZERO_LENGTH message.",__FUNCTION__);
        snprintf(msg, msg_size-1, "{\"proxyId\": \"%s\", \"sequenceNumber\": \"%lu\"}", device_id, number);
    }
    else {
        char buf[LIB_HTTP_MAX_MSG_SIZE*2];
        unsigned int i = 0;

        while((msg[i++] != '{') && (i < strlen(msg)));

        if(i >= strlen(msg)) {
            pu_log(LL_DEBUG,"%s: ===== 3 i=%d",__FUNCTION__,i);
            snprintf(buf, sizeof(buf) - 1, "{\"proxyId\": \"%s\", \"sequenceNumber\": \"%lu\"}", device_id, number);
        }
        else {
            pu_log(LL_DEBUG,"%s: ===== 4 i=%d  ",__FUNCTION__,i);
            snprintf(buf, sizeof(buf)-1, "{\"proxyId\": \"%s\", \"sequenceNumber\": \"%lu\", %s", device_id, number, msg+1);
        }

        strncpy(msg, buf, msg_size-1);
        msg[msg_size-1] = '\0';
    }
    pu_log(LL_ERROR,"%s:===== 5 resulting message: %'s'",__FUNCTION__,msg);
    return strlen(msg);
}