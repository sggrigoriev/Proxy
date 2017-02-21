//
// Created by gsg on 13/12/16.
//

#include <stdio.h>
#include <string.h>
#include <cJSON.h>

#include "lib_http.h"
#include "pu_logger.h"
#include "pf_traffic_proc.h"

// {"proxtId":"<deviceId>, "sqeuenceNunber": <sqe_number>, <msg>}
size_t pf_add_proxy_head(char* msg, size_t msg_size, const char* device_id, unsigned int seq_number) {

    if(!strlen(msg)) {
        snprintf(msg, msg_size, "{\"proxyId\": \"%s\", \"sequenceNumber\": \"%d\"}", device_id, seq_number);
    }
    else {
        char buf[LIB_HTTP_MAX_MSG_SIZE*2];
        unsigned int i = 0;

        while((msg[i++] != '{') && (i < strlen(msg)));

        if(i >= strlen(msg)) i = 0;

        snprintf(buf, sizeof(buf)-1, "{\"proxyId\": \"%s\", \"sequenceNumber\": \"%d\", %s}", device_id, seq_number, msg+i);

        strncpy(msg, buf, msg_size-1);
        msg[msg_size-1] = '\0';
    }

    return strlen(msg);
}
