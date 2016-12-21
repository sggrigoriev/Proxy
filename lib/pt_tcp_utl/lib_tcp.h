//
// Created by gsg on 19/12/16.
//
// Theese functions are for multiple reads from several connection
//

#ifndef PRESTO_LIB_TCP_H
#define PRESTO_LIB_TCP_H

#include <stdio.h>

#define LIB_TCP_BINGING_ATTEMPTS 10

typedef struct {
    int* sock_array;    //-1 if not in use
    unsigned int start_no;
    unsigned int size;
} lib_tcp_conn_t;

//Return NULL if allocation error
lib_tcp_conn_t* lib_tcp_init_conns(unsigned int max_connections);
//Return ptr for upfated descriptor ot NULL if no space
lib_tcp_conn_t* lib_tcp_add_new_conn(int rd_socket, lib_tcp_conn_t* all_conns);
//return 1 if there is at least one connected sockets available
int lib_tcp_are_conn(lib_tcp_conn_t* all_conns);
void lib_tcp_destroy_conns(lib_tcp_conn_t* all_conns);
//All three return -1 if error, 0 if timeout, >0 if value
//return binded socket
int lib_tcp_get_server_socket(int port);
//return connected socker for R/W operations
int lib_tcp_listen(int server_socket, int to_sec);
//return bytes red
ssize_t lib_tcp_read(lib_tcp_conn_t* all_conns, char* in_buf, size_t size, int to_sec);


#endif //PRESTO_LIB_TCP_H
