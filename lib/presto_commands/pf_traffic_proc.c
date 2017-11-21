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

#include "pu_logger.h"
#include "lib_http.h"

#include "pf_traffic_proc.h"

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

    if(!strlen(msg)) {
        snprintf(msg, msg_size-1, "{\"proxyId\": \"%s\", \"sequenceNumber\": \"%lu\"}", device_id, number);
    }
    else {
        char buf[LIB_HTTP_MAX_MSG_SIZE*2];
        unsigned int i = 0;

        while((msg[i++] != '{') && (i < strlen(msg)));

        if(i >= strlen(msg)) {
            snprintf(buf, sizeof(buf) - 1, "{\"proxyId\": \"%s\", \"sequenceNumber\": \"%lu\"}", device_id, number);
        }
        else {
            snprintf(buf, sizeof(buf)-1, "{\"proxyId\": \"%s\", \"sequenceNumber\": \"%lu\", %s", device_id, number, msg+1);
        }

        strncpy(msg, buf, msg_size-1);
        msg[msg_size-1] = '\0';
    }
    return strlen(msg);
}