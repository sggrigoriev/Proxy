//
// Created by gsg on 13/11/16.
//
// Contains data and functions to communicate with the Cloud

#ifndef PT_HTTP_UTL_H
#define PT_HTTP_UTL_H

int pt_http_curl_init();        //Global cUrl init. Returns 0 if failed
void pt_http_curl_stop();

int pt_http_read_reconnect();    // Returns 0 if failed
int pt_http_write_reconnect();   // Returns 0 if failed

int pt_http_read_init();     // Returns 0 if something wrong
int pt_http_write_init();    // Returns 0 if something wrong

void pt_http_read_destroy();
void pt_http_write_destroy();


int pt_http_read(char** in_buf); // Returns 0 if timeout or actual buf len (LONG GET), < 0 if error

int pt_http_write(char* buf, unsigned int len, char** resp, unsigned int* resp_len); //Returns 0 if timeout or error, 1 if OK
#endif //PT_HTTP_UTL_H
