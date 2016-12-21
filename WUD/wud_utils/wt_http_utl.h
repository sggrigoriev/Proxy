//
// Created by gsg on 07/12/16.
//
// Copypizded from pt_http_utl. All unnecessary functionality cut down

#ifndef PRESTO_WT_HTTP_UTL_H
#define PRESTO_WT_HTTP_UTL_H

#include "lib_http.h"

int wt_http_curl_init();        //Returns 0 if failed 1 if succeed
void wt_http_curl_stop();       //Returns 0 if failed 1 if succeed

lib_http_post_result_t wt_http_write(char* buf, size_t buf_size, char* resp, size_t resp_size); //Returns 0 if timeout, -1 if error, 1 if OK

int wt_http_gf_init();
void wt_http_gf_destroy();

int wt_http_get_file(const char* connection_string, const char* folder); //Returns 0 if timeout or error, 1 if OK

void wt_set_connection_info(const char* conn_string, const char* device_id, const char* activation_token);

char* wt_http_get_cloud_conn_string(char *buf, size_t len);

char* wt_get_device_id(char* buf, size_t len);

#endif //PRESTO_WT_HTTP_UTL_H
