//
// Created by gsg on 19/12/16.
//
// Theese functions are for multiple reads from several connection
//

#ifndef PRESTO_LIB_TCP_H
#define PRESTO_LIB_TCP_H

#include <stdio.h>

#define LIB_TCP_BINGING_ATTEMPTS 100
#define LIB_TCP_READ_TIMEOUT (lib_tcp_rd_t*)1L
#define LIB_TCP_READ_MSG_TOO_LONG (lib_tcp_rd_t*)2L
#define LIB_TCP_READ_NO_READY_CONNS (lib_tcp_rd_t*)3L

typedef struct{
    char* buf;
    size_t size;
    size_t idx;
} lib_tcp_assembling_buf_t;

typedef struct {
    char* buf;
    size_t size;
    ssize_t len;
} lib_tcp_in_t;

typedef struct {
    int socket;
    lib_tcp_in_t in_buf;
    lib_tcp_assembling_buf_t ass_buf;
} lib_tcp_rd_t;

typedef struct {
    lib_tcp_rd_t* rd_conn_array;    //-1 if not in use
    unsigned int start_no;
    unsigned int sa_size;
    unsigned int sa_max_size;
} lib_tcp_conn_t;

//Return NULL if allocation error
lib_tcp_conn_t* lib_tcp_init_conns(unsigned int max_connections, size_t in_size, size_t ass_size);

//Return ptr for upfated descriptor ot NULL if no space
lib_tcp_conn_t* lib_tcp_add_new_conn(int rd_socket, lib_tcp_conn_t* all_conns);

//return amount of connected sockets
int lib_tcp_conn_amount(lib_tcp_conn_t* all_conns);

void lib_tcp_destroy_conns(lib_tcp_conn_t* all_conns);

//All three return -1 if error, 0 if timeout, >0 if value
//return binded socket
int lib_tcp_get_server_socket(int port);

//return connected socket for R/W operations, 0 if timeout and negative if error
int lib_tcp_listen(int server_socket, int to_sec);

//return connection desriptor or NULL if error
lib_tcp_rd_t* lib_tcp_read(lib_tcp_conn_t* all_conns, int to_sec);

//Return the 0-terminated message of NULL if no finished string
const char* lib_tcp_assemble(lib_tcp_rd_t* conn, char* out, size_t out_size);

//All three return -1 if error, 0 if timeout, >0 if value
//return binded socket
int lib_tcp_get_client_socket(int port, int to_sec);

//Return bytes sent amt, 0 if timeout, < 0 if error
ssize_t lib_tcp_write(int wr_socket, const char* out, size_t size, int to_sec);

void lib_tcp_client_close(int write_socket);

#endif //PRESTO_LIB_TCP_H
