//
// Created by gsg on 13/11/16.
//
// Contains data and functions to communicate with the Cloud

#ifndef PT_HTTP_UTL_H
#define PT_HTTP_UTL_H

int pt_http_curl_init();        //Returns 0 if failed 1 if succeed
void pt_http_curl_stop();       //Returns 0 if failed 1 if succeed

int pt_http_read_init();     //Returns 0 if failed 1 if succeed
int pt_http_write_init();    //Returns 0 if failed 1 if succeed

void pt_http_read_destroy();
void pt_http_write_destroy();


int pt_http_read(char** in_buf); // Returns 0 if timeout or actual buf len (LONG GET), < 0 if error- no need to continue retries (-errno)

typedef enum{PT_POST_ERROR = -1, PT_POST_RETRY = 0, PT_POST_OK = 1} pr_post_result_t;
//Returns 0 if timeout, -1 or error, 1 if OK
//In all cases when the return is <=0 the resp contains some diagnostics
pr_post_result_t pt_http_write(char* buf, size_t len, char** resp, size_t* resp_len);
#endif //PT_HTTP_UTL_H
