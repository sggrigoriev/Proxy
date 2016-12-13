//
// Created by gsg on 13/12/16.
//

#include <pc_defaults.h>
#include <pc_settings.h>
#include <string.h>
#include "pf_traffic_proc.h"


size_t pf_add_proxy_head(char* msg, size_t len) {
    char buf[PROXY_MAX_MSG_LEN*2];
    char devID[PROXY_DEVICE_ID_SIZE];
    unsigned int i = 0;
    while((msg[i++] != '{') && (i < strlen(msg)));

    if(i >= strlen(msg)) i = 0;

    pc_getDeviceAddress(devID, sizeof(devID)-1);

    snprintf(buf, sizeof(buf)-1, "{\"proxyId\": \"%s\", %s}", devID, msg+i);

    strncpy(msg, buf, len-1);
    return strlen(msg);
}
