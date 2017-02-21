//
// Created by gsg on 30/01/17.
//

#ifndef PRESTO_WH_MANAGER_H
#define PRESTO_WH_MANAGER_H

#include <stdio.h>

#include "lib_http.h"
//Init cURL and create connections pul size 2: - 1 for POST and 1 for file GET;
//Make POST connection
void wh_mgr_start();
//Close cURL & erase connections pool
void wh_mgr_stop();
//(re)create post connection; update conn parameters
void wh_reconnect(const char* new_url, const char* new_auth_token);


//Returns 0 if error, 1 if OK
int wh_write(char* buf, char* resp, size_t resp_size);
//Returns 0 if error, 1 if OK
int wh_read_file(const char* file_with_path,  const char* url, unsigned int attempts_amount);

#endif //PRESTO_WH_MANAGER_H
