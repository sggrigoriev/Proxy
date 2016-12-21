//
// Created by gsg on 08/11/16.
//

#include <string.h>
#include <pthread.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <stdlib.h>
#include "unistd.h"
#include <netinet/in.h>
#include <fcntl.h>
#include <assert.h>

//#include "pc_defaults.h"
#include "pu_logger.h"
#include "pt_tcp_utl.h"
#include "lib_tcp.h"


//////////////////////////////////////////////////////////////////
//Server part
//
/////////////////////////////////////////////////////////////////
//pt_tcp_server_connect - establish server connection to the port and make dup for write
//
//Return 1 if OK and 0 if error
int pt_tcp_server_connect(int port, pt_tcp_rw_t* rw_sockets) {  // returns socket or -1 if error
//Create server socket
    int server_socket;

    if (server_socket = socket(AF_INET, SOCK_STREAM, 0), server_socket  < 0) {
        pu_log(LL_ERROR, "pt_tcp_server_connect: error socket creation %d, %s", errno, strerror(errno));
        return 0;
    }
//Set socket options
    int32_t on = 1;
    //use the socket even if the address is busy (by previously killed process for ex)
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &on, sizeof (on)) < 0) {
        pu_log(LL_ERROR, "pt_tcp_server_connect: error configuring connection %d, %s", errno, strerror(errno));
        return 0;
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
        pu_log(LL_ERROR, "pt_tcp_server_connect: error socket binding %d, %s", errno, strerror(errno));
        sleep(1);
        if (!rpt--) return -1;
    }
 //Make communication sockets
//listen for incoming connection
    if (listen(server_socket, 1) < 0) {
        pu_log(LL_ERROR, "pt_tcp_server_connect: error listen incoming connection %d, %s", errno, strerror(errno));
        return 0;
    }
/* Call select() */
    int result = 0;
    fd_set readset;
    while(!result) {
        FD_ZERO(&readset);
        FD_SET(server_socket, &readset);
        struct timeval tv = {PT_SOCK_SELECT_TO_SEC, 0};
        result = select(server_socket + 1, &readset, NULL, NULL, &tv);
        if (result == 0) { // Timeout happens. nothing to read
             continue;
        }
        if (result < 0) {
            pu_log(LL_ERROR, "pt_tcp_server_connect: error on select read %d, %s", errno, strerror(errno));
            return 0;
        }
// We got something to read!
        if (FD_ISSET(server_socket, &readset)) { // Strange check but the manual asks for it
//accept incoming connection
            struct sockaddr_in cli_addr = {0};
            socklen_t size = sizeof(struct sockaddr);
            int conn_sock = accept(server_socket, (struct sockaddr *) &cli_addr, &size);
            if (conn_sock < 0) {
                pu_log(LL_ERROR, "pt_tcp_server_connect: error accepting incoming connection %d, %s", errno, strerror(errno));
                return 0;
            }

            rw_sockets->server_socket = server_socket;
            rw_sockets->rd_socket = conn_sock;
            rw_sockets->wr_socket = dup(rw_sockets->rd_socket);
            return 1;
        }
        else {
            pu_log(LL_ERROR, "pt_tcp_server_connect: FD_ISSET error %d, %s", errno, strerror(errno));
        }
    }
    return 0;
}
//////////////////////////////////////////////////
//Client part
//
//////////////////////////////////////////////////
//pt_tcp_client_connect - establish client onnection to the port
//
//port  - port to connect to
//Return connected socket (>0), -1 if error
int pt_tcp_client_connect(int port, pt_tcp_rw_t* rw_sockets) { // returns socket or -1 if error
    int client_socket;
// Create socket
    if (client_socket = socket(AF_INET, SOCK_STREAM, 0), client_socket < 0) {
        pu_log(LL_ERROR, "pt_tcp_client_connect: error socket creation %d, %s", errno, strerror(errno));
        return 0;
    }
    //Set socket options
    int32_t on = 1;
    //use the socket even if the address is busy (by previously killed process for ex)
    if (setsockopt(client_socket, SOL_SOCKET, SO_REUSEADDR, &on, sizeof (on)) < 0) {
        pu_log(LL_ERROR, "pt_tcp_client_connect: error configuring connection %d, %s", errno, strerror(errno));
        return 0;
    }

//Make address
    struct sockaddr_in addr_struct;
    memset(&addr_struct, 0, sizeof(addr_struct));
    addr_struct.sin_family = AF_INET;
    addr_struct.sin_addr.s_addr = inet_addr("127.0.0.1");
    addr_struct.sin_port = htons(port);

//And connect to the remote socket
    unsigned rpt = LIB_TCP_BINGING_ATTEMPTS;
    while(rpt) {
        int ret = connect(client_socket, (struct sockaddr *)&addr_struct, sizeof(addr_struct));
        if (ret < 0) {
            pu_log(LL_ERROR, "pt_tcp_client_connect: error connection %d, %s", errno, strerror(errno));
            rpt--;
            sleep(1);   //wait for a while
        }
        else {
            rw_sockets->server_socket = -1;
            rw_sockets->rd_socket = client_socket;
            rw_sockets->wr_socket = dup(rw_sockets->rd_socket);
            return 1;
        }
    }
    return 0;
}
/////////////////////////////////////////////////
//pt_tcp_shutdown   - close read/write connections
//
//sock      - socket to close
void pt_tcp_shutdown_rw(pt_tcp_rw_t* socks) {
    if(socks->rd_socket >= 0) {
        shutdown(socks->rd_socket, SHUT_RDWR);
        close(socks->rd_socket);
    }
    if(socks->wr_socket >= 0) {
        shutdown(socks->wr_socket, SHUT_RDWR);
        close(socks->wr_socket);
    }
    if(socks->server_socket >= 0) {
        shutdown(socks->server_socket, SHUT_RDWR);
        close(socks->server_socket);
     }
}
/////////////////////////////////////////////////
//pt_tcp_assemble   - assembling message from several tcp parsels if for some reasons the message
//                  wasn't sent as one piece. The sign of end message is the 0-byte
//
//Return the 0-terminated message or NULL if empty
const char* pt_tcp_assemble(char* out, size_t out_size, pt_tcp_assembling_buf_t* ab) { // assimbling the full message. Return NULL if nothing oe msg
    unsigned i;
    assert(out);
    assert(ab);
    assert(out_size);

    for(i = 0; i < ab->idx; i++) {
        if(ab->buf[i] == '\0') {
            memcpy(out, ab->buf, i+1);
            memmove(ab->buf, ab->buf+i+1, ab->idx-(i+1));
            ab->idx = ab->idx-(i+1);
            return out;
        }
    }
    return NULL;
}
int pt_tcp_get(const char* in, size_t len, pt_tcp_assembling_buf_t* ab) {
    assert(in); assert(ab); assert(len);

    if((ab->idx >= ab->buf_len) || ((ab->buf_len - ab->idx) < len)) return 0; //no place in buffer
    memcpy(ab->buf+ab->idx, in, len);
    ab->idx += len;
    return 1;
}