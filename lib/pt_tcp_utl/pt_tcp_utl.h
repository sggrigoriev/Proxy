//
// Created by gsg on 08/11/16.
//

#ifndef PT_TCP_UTL_H
#define PT_TCP_UTL_H

#include <stdio.h>

#define PT_BINDING_ATTEMPTS     1000
#define PT_SOCK_SELECT_TO_SEC   1

typedef struct {int server_socket; int rd_socket; int wr_socket;} pt_tcp_rw_t;

//Server part
int pt_tcp_server_connect(int port, pt_tcp_rw_t* rw_sockets);    //Return 1 if OK 0 if not
//////////////////////////////////////////////////
//Client part
int pt_tcp_client_connect(int port, pt_tcp_rw_t* rw_sockets);                    ///Return 1 if OK 0 if not
//////////////////////////////////////////////////
void pt_tcp_shutdown_rw(pt_tcp_rw_t* socks);
//

typedef struct{
    char* buf;
    size_t buf_len;
    size_t idx;
} pt_tcp_assembling_buf_t;

//Return the 0-terminated message of NULL if no finished string
const char* pt_tcp_assemble(char *out, size_t out_len, pt_tcp_assembling_buf_t* assembling_buf);
//Return 1 if ok, return 0 if in was rejected
int pt_tcp_get(const char* in, ssize_t len, pt_tcp_assembling_buf_t* ab);
#endif //PT_TCP_UTL_H
