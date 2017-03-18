/*
 *  Copyright 2017 People Power Company
 *
 *  This code was developed with funding from People Power Company
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */
/*
 Created by gsg on 19/12/16.

 wrapper under standard tcp Linux interface for async read.
NB1! all messages for read & write should be 0-terminated strings!
NB2! Interface functions are thread-protected. But still async...
*/

#ifndef PRESTO_LIB_TCP_H
#define PRESTO_LIB_TCP_H

#include <stdio.h>

#define LIB_TCP_BINGING_ATTEMPTS 1

/*Errors returned instead of connection handler. Not a good practice... */
#define LIB_TCP_READ_TIMEOUT (lib_tcp_rd_t*)1L          /*Read timeout */
#define LIB_TCP_READ_MSG_TOO_LONG (lib_tcp_rd_t*)2L     /*Too big message on input */
#define LIB_TCP_READ_NO_READY_CONNS (lib_tcp_rd_t*)3L   /*Connection pool is full - no place for new connections */
#define LIB_TCP_READ_EOF (lib_tcp_rd_t*)4L              /*Connection is broken */

/* Status to prevent buffer overflow:
 * LIB_TCP_BUF_READY        - ready to accespt new message
 * LIB_TCP_BUF_IN_PROCESS   - ready to take new portion of message - '\0' is not found
 * LIB_TCP_BUF_REJECT       - too long message - ignore it until the '\0' will happened
*/
typedef enum {LIB_TCP_BUF_READY, LIB_TCP_BUF_IN_PROCESS, LIB_TCP_BUF_REJECT} lib_tcp_ass_buf_state_t;

/*Buffer for assembling the whole 0-terminated string.
Made because TCP could pass just the part of the message in case of high load
*/
typedef struct{
    char* buf;
    size_t size;
    size_t idx;
    lib_tcp_ass_buf_state_t status;

} lib_tcp_assembling_buf_t;

/*Incoming buffer type */
typedef struct {
    char* buf;
    size_t size;
    int len;/*mlevitin ssize_t len; */
} lib_tcp_in_t;

/*Read descriptor */
typedef struct {
    int socket;
    lib_tcp_in_t in_buf;
    lib_tcp_assembling_buf_t ass_buf;
} lib_tcp_rd_t;

/*TCP read connection descriptor */
typedef struct {
    lib_tcp_rd_t* rd_conn_array;    /*-1 if not in use */
    unsigned int start_no;
    unsigned int sa_size;
    unsigned int sa_max_size;
} lib_tcp_conn_t;

/*Create the connection pool
  max_connections - size of connections pool
  in_size         - max size of incoming data read for one time
  ass_size        - size of assembling buffer (usually in_size*2
Return NULL if allocation error or pointer to connections pool
*/
lib_tcp_conn_t* lib_tcp_init_conns(unsigned int max_connections, size_t in_size);

/*Create working connecion
  rd_socket   - TCP socket open for read
  all_conns   - pointer connection pool
Return ptr for upfated descriptor ot NULL if no space
*/
lib_tcp_conn_t* lib_tcp_add_new_conn(int rd_socket, lib_tcp_conn_t* all_conns);

/*Get the amount of working connections
  all_conns   - pointer to connections pool
Return amount of connected sockets
*/
int lib_tcp_conn_amount(lib_tcp_conn_t* all_conns);

/*Close all open connections and erase the connections pool
  all_conns   - pointer to connections pool
*/
void lib_tcp_destroy_conns(lib_tcp_conn_t* all_conns);

/*Create server socket connected to the port
  port    - the port connected to
Return binded socket or -1 if error
*/
int lib_tcp_get_server_socket(int port);

/*Listen for incoming connections
  server_socket   - binded server socket
  to_sec          - timeout in seconds waiting for new connections
Return connected socket for R/W operations, 0 if timeout and -1 if error
*/
int lib_tcp_listen(int server_socket, int to_sec);

/*Get the first of all ready to be red connections
  all_conns   - pointer to connections pool
  to_sec      - timeout to wait for ready for read connections
Return connection desriptor or one of these funny defines described on top of the file
*/
lib_tcp_rd_t* lib_tcp_read(lib_tcp_conn_t* all_conns, int to_sec);

/*Assemble the incoming chnks to the 0-termineted char string and move it into out buffer
  conn    - connection descriptor
  out     - buffer for incoming message
  size    - size of incoming buffer
Return the pointer to the out or NULL if no finished string
*/
const char* lib_tcp_assemble(lib_tcp_rd_t* conn, char* out, size_t out_size);

/*Create client socket connected to the port
  port    - the port to connect to
  to_sec  - timeout for connection establishment
Return binded socket or -1 if error
*/
int lib_tcp_get_client_socket(int port, int to_sec);

/*Write the string to the socket
  wr_socekt   - writable socket
  out         - the data to be written (0-terminated string)
  size        - string size (including 0-byte) -- some old ideas... Actually strlen(out) would be enough
  to_sec      - timeout for write operation
Return bytes sent amt, 0 if timeout, < 0 if error
*/
int lib_tcp_write(int wr_socket, const char* out, size_t size, int to_sec);

/*Close writable socket
  write_socket    - writable socket to be closed
*/
void lib_tcp_client_close(int write_socket);

#endif /*PRESTO_LIB_TCP_H*/
