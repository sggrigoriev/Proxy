//
// Created by gsg on 19/12/16.
//

#include <assert.h>
#include <malloc.h>
#include <string.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>

#include "lib_tcp.h"


lib_tcp_conn_t* lib_tcp_init_conns(unsigned int max_connections) {
    assert(max_connections);
    lib_tcp_conn_t* ret = malloc(sizeof(lib_tcp_conn_t));
    if(!ret) return NULL;
    ret->sock_array = malloc(max_connections* sizeof(int));
    if(!ret->sock_array) return NULL;

    for(unsigned int i = 0; i < max_connections; i++) ret->sock_array[i]= -1;
    ret->size = max_connections;
    ret->start_no = 0;
    return ret;
}
lib_tcp_conn_t* lib_tcp_add_new_conn(int rd_socket, lib_tcp_conn_t* all_conns) {
    assert(all_conns);
    for(unsigned int i = 0; i < all_conns->size; i++) {
        if(all_conns->sock_array[i] < 0) {
            all_conns->sock_array[i] = rd_socket;
            return all_conns;
        }
    }
    return NULL;
}
int lib_tcp_are_conn(lib_tcp_conn_t* all_conns) {

    for(unsigned int i = 0; i < all_conns->size; i++)
        if(all_conns->sock_array[i] >= 0) return 1;
    return 0;
}
void lib_tcp_destroy_conns(lib_tcp_conn_t* all_conns) {
    if(!all_conns) return;
    for(unsigned int i = 0; i < all_conns->size; i++) {
        if(all_conns->sock_array[i] >= 0) {
            shutdown(all_conns->sock_array[i], SHUT_RDWR);
            close(all_conns->sock_array[i]);
        }
    }
    free(all_conns->sock_array);
    free(all_conns);
}
static void remove_conn(lib_tcp_conn_t* all_conns, int socket) {
    for(unsigned int i = 0; i < all_conns->size; i++) {
        if(all_conns->sock_array[i] == socket) {
            close(socket);
            all_conns->sock_array[i] = -1;
        }
    }
}
static unsigned int inc_start_no(unsigned int no, unsigned int size) {
    return (++no < size)?no:0;
}
//All three return -1 if error, 0 if timeout, >0 if value
//return binded socket
int lib_tcp_get_server_socket(int port) {
    //Create server socket
    int server_socket;

    if (server_socket = socket(AF_INET, SOCK_STREAM|SOCK_NONBLOCK, 0), server_socket  < 0) {
        return -1;
    }
//Set socket options
    int32_t on = 1;
    //use the socket even if the address is busy (by previously killed process for ex)
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &on, sizeof (on)) < 0) {
        return -1;
    }
//Make address
    struct sockaddr_in addr_struct;
    memset(&addr_struct, 0, sizeof(addr_struct));
    addr_struct.sin_family = AF_INET;
    addr_struct.sin_addr.s_addr = INADDR_ANY;
    addr_struct.sin_port = htons(port);

//bind socket on repeated mode
    int rpt = LIB_TCP_BINGING_ATTEMPTS;
    while (bind(server_socket, (struct sockaddr *) &addr_struct, sizeof(addr_struct)) < 0 ) {
        sleep(1);
        if (!rpt--) return -1;
    }
    return server_socket;
}
//return connected socket for R/W operations
int lib_tcp_listen(int server_socket, int to_sec) {
    //Make communication sockets
//listen for incoming connection
    if (listen(server_socket, 1) < 0) {
        return -1;
    }
/* Call select() */
    fd_set readset;
    FD_ZERO(&readset);
    FD_SET(server_socket, &readset);
    struct timeval tv = {to_sec, 0};

    int result = select(server_socket + 1, &readset, NULL, NULL, &tv);
    if(result <= 0) return result; // Timeoutor error. nothing to read
// We got something to read!
//accept incoming connection
    struct sockaddr_in cli_addr = {0};
    socklen_t size = sizeof(struct sockaddr);
    int conn_sock = accept(server_socket, (struct sockaddr *) &cli_addr, &size);
    if (conn_sock < 0) return -1;

    int sock_flags = fcntl(conn_sock, F_GETFL);
    if (sock_flags < 0) {
        return -1;
    }
    if (fcntl(conn_sock, F_SETFL, sock_flags|O_NONBLOCK) < 0) {
        return -1;
    }
    return conn_sock;
}
static int make_fdset(lib_tcp_conn_t* all_conns, fd_set* fds) {
    int max_fd = -1;
    FD_ZERO(fds);
    for(unsigned int i = 0; i < all_conns->size; i++) { // get all valid sockets
        if(all_conns->sock_array[i] > 0) FD_SET(all_conns->sock_array[i], fds);
        if(all_conns->sock_array[i] > max_fd) max_fd = all_conns->sock_array[i];
    }
    return max_fd;
}
static int get_ready_sock(lib_tcp_conn_t* all_conns, fd_set* fds) {
    unsigned int idx = all_conns->start_no;
    int ret = -1;
    for(unsigned int i = 0; (i < all_conns->size)&&(ret < 0); i++) {
        if((all_conns->sock_array[idx] >= 0) && FD_ISSET(all_conns->sock_array[idx], fds)) ret = all_conns->sock_array[idx];
        idx = inc_start_no(idx, all_conns->size);
    }
    all_conns->start_no = inc_start_no(all_conns->start_no, all_conns->size);
    return ret;
}
//return bytes red
ssize_t lib_tcp_read(lib_tcp_conn_t* all_conns, char* in_buf, size_t size, int to_sec) {
    assert(all_conns); assert(in_buf); assert(size);
//Build set for select
    struct timeval tv = {to_sec, 0};
    fd_set readset;
    int max_fd = make_fdset(all_conns, &readset);
    if(max_fd < 0) return max_fd;
    int result = select(max_fd + 1, &readset, NULL, NULL, &tv);
    if(result <= 0) return result; // Timeoutor error. nothing to read

    int sock = get_ready_sock(all_conns, &readset);
    if(sock < 0) return -1; //no ready sockets error - they sould be!

    ssize_t ret = read(sock, in_buf, size);
    if(ret >= 0) return ret;
//Get read error - reconnect required
    remove_conn(all_conns, result);
    return -1;
}

