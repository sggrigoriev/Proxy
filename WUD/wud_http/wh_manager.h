//
// Created by gsg on 30/01/17.
//

#ifndef PRESTO_WH_MANAGER_H
#define PRESTO_WH_MANAGER_H

#include "lib_http.h"
//Init cURL and create 1 connection for post
void wh_mgr_start();
//Close cURL & erase connections pool
void wh_mgr_stop();
//(re)create post connection; update conn parameters
void wh_connect(const char* connection_string, const char* device_id, const char* activation_token);

//Returns 0 if error, 1 if OK
int wh_write(char* buf, char* resp, size_t resp_size);


#endif //PRESTO_WH_MANAGER_H
