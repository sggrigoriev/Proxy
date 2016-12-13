//
// Created by gsg on 13/12/16.
//
//Contains all functions for Proxy traffic analysis and upate
//

#ifndef PRESTO_PF_TRAFFIC_PROC_H
#define PRESTO_PF_TRAFFIC_PROC_H

#include <stdlib.h>
// {"proxtId":"<deviceId>", <msg>}
size_t pf_add_proxy_head(char* msg, size_t len);

#endif //PRESTO_PF_TRAFFIC_PROC_H
