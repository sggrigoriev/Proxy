//
// Created by gsg on 13/11/16.
//
// Contains data and functions to communicate with the Cloud

#ifndef PT_HTTP_UTL_H
#define PT_HTTP_UTL_H

#include "lib_http.h"

int pt_http_curl_start();        //Returns 0 if failed 1 if succeed
void pt_http_curl_stop();       //Returns 0 if failed 1 if succeed

int pt_http_read_init();
void pt_http_read_destroy();

// Returns 0 if timeout, 1 if OK, -1 if error- no need to continue retries
int pt_http_read(char* in_buf, size_t max_len);

//Returns 0 if timeout, -1 or error, 1 if OK
//In all cases when the return is <=0 the resp contains some diagnostics
lib_http_post_result_t pt_http_write(char* buf, size_t buf_size, char* resp, size_t resp_size);
#endif //PT_HTTP_UTL_H
