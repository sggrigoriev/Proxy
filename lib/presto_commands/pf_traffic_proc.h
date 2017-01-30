//
// Created by gsg on 13/12/16.
//
//Contains all functions for Proxy traffic analysis and upate
//

#ifndef PRESTO_PF_TRAFFIC_PROC_H
#define PRESTO_PF_TRAFFIC_PROC_H

#include <stdlib.h>
// {"proxtId":"<deviceId>, "sqeuenceNunber": <sqe_number>", <msg>}
size_t pf_add_proxy_head(char* msg, size_t len, const char* device_id, unsigned int seq_number);

#endif //PRESTO_PF_TRAFFIC_PROC_H
