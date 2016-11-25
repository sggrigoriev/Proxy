//
// Created by gsg on 08/11/16.
//

#include <string.h>
#include <pthread.h>
#include <errno.h>
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include "unistd.h"
#include <netinet/in.h>
#include <fcntl.h>
#include <assert.h>

#include "pc_defaults.h"
#include "pu_logger.h"
#include "pt_tcp_utl.h"


//////////////////////////////////////////////////////////////////
//Server part
//
static int server_socket;
/////////////////////////////////////////////////////////////////
//pt_tcp_server_connect - establish server connection to the port or re-establish server connection to the port
//
//port      - port to connect to
//sock      - socket to close in case of reconnect
//reconnect - 0 - make new connection, 1 - close the socket and connect again
//Return connected socket (>0), -1 if error
int pt_tcp_server_connect(int port, int sock, int reconnect) {  // returns socket or -1 if error
    if(!reconnect) {
//Create socket
        if (server_socket = socket(AF_INET, SOCK_STREAM, 0), server_socket  < 0) {
            pu_log(LL_ERROR, "pt_tcp_server_connect: error socket creation %d, %s", errno, strerror(errno));
            return -1;
        }

//Set socket options
        int32_t on = 1;
        //use the socket even if the address is busy (by previously killed process for ex)
        if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &on, sizeof (on)) < 0) {
            pu_log(LL_ERROR, "pt_tcp_server_connect: error configuring connection %d, %s", errno, strerror(errno));
            return -1;
        }

//Make address
        struct sockaddr_in addr_struct;
        memset(&addr_struct, 0, sizeof(addr_struct));
        addr_struct.sin_family = AF_INET;
        addr_struct.sin_addr.s_addr = INADDR_ANY;
        addr_struct.sin_port = htons(port);

//bind socket on repeated mode
        int rpt = DEFAULT_BINDING_ATTEMPTS;
        while (bind(server_socket, (struct sockaddr *) &addr_struct, sizeof(addr_struct)) < 0 ) {
            pu_log(LL_ERROR, "pt_tcp_server_connect: error socket binding %d, %s", errno, strerror(errno));
            sleep(1);
            if (!rpt--) return -1;
        }
    }

//Common part for connect & reconnect
    if (sock >= 0) close(sock);

//listen for incoming connection
    if (listen(server_socket, 1) < 0) {
        pu_log(LL_ERROR, "pt_tcp_server_connect: error listen incoming connection %d, %s", errno, strerror(errno));
        return -1;
    }
/* Call select() */
    int result = 0;
    fd_set readset;
    while(!result) {
        FD_ZERO(&readset);
        FD_SET(server_socket, &readset);
        struct timeval tv = {DEFAULT_SOCK_SELECT_TO_SEC, 0}; // timeout for one second
        result = select(server_socket + 1, &readset, NULL, NULL, &tv);
        if (result == 0) { // Timeout happens. nothing to read
             continue;
        }
        if (result < 0) {
            pu_log(LL_ERROR, "pt_tcp_server_connect: error on select read %d, %s", errno, strerror(errno));
            return -1;
        }
// We got something to read!
        if (FD_ISSET(server_socket, &readset)) { // Strange check but the manual asks for it
//accept incoming connection
            struct sockaddr_in cli_addr = {0};
            socklen_t size = sizeof(struct sockaddr);
            int conn_sock = accept(server_socket, (struct sockaddr *) &cli_addr, (socklen_t *) &size);
            if (conn_sock < 0) {
                pu_log(LL_ERROR, "pt_tcp_server_connect: error accepting incoming connection %d, %s", errno, strerror(errno));
                return -1;
            }
            int ret = fcntl(conn_sock, F_GETFL);
            if (ret < 0) {
                pu_log(LL_ERROR, "pt_tcp_server_connect: error get sock opts %d, %s", errno, strerror(errno));
                return -1;
            }
            if (fcntl(conn_sock, F_SETFL, ret|O_NONBLOCK) < 0) {
                pu_log(LL_ERROR, "pt_tcp_server_connect: error set NONBLOCK sock opts %d, %s", errno, strerror(errno));
                return -1;
            }
            return conn_sock;
        }
    }
    return -1;
}
//////////////////////////////////////////////////
//Client part
//
//////////////////////////////////////////////////
//pt_tcp_client_connect - establish client onnection to the port or re-establish client connection to the port
//
//port  - port to connect to
//sock  - if sock < 0 - new connect if sock >=0 thean soc is previously opened socket
//Return connected socket (>0), -1 if error
int pt_tcp_client_connect(int port, int sock) { // returns socket or -1 if error

    int client_socket;
    if(sock >= 0) close(sock);  //Case of reconnect
// Create socket
    if (client_socket = socket(AF_INET, SOCK_STREAM, 0), client_socket < 0) {
        pu_log(LL_ERROR, "pt_tcp_client_connect: error socket creation %d, %s", errno, strerror(errno));
        return -1;
    }
    //Set socket options
    int32_t on = 1;
    //use the socket even if the address is busy (by previously killed process for ex)
    if (setsockopt(client_socket, SOL_SOCKET, SO_REUSEADDR, &on, sizeof (on)) < 0) {
        pu_log(LL_ERROR, "pt_tcp_client_connect: error configuring connection %d, %s", errno, strerror(errno));
        return -1;
    }

//Make address
    struct sockaddr_in addr_struct;
    memset(&addr_struct, 0, sizeof(addr_struct));
    addr_struct.sin_family = AF_INET;
    addr_struct.sin_addr.s_addr = INADDR_ANY;
    addr_struct.sin_port = htons(port);

//And connect to the remote socket
    unsigned rpt = DEFAULT_BINDING_ATTEMPTS;
    while(rpt) {
        int ret = connect(client_socket, (struct sockaddr *)&addr_struct, sizeof(addr_struct));
        if (ret < 0) {
            pu_log(LL_ERROR, "pt_tcp_client_connect: error connection %d, %s", errno, strerror(errno));
            rpt--;
            sleep(1);   //wait for a while
        }
        else {
            int ret = fcntl(client_socket, F_GETFL);
            if (ret < 0) {
                pu_log(LL_ERROR, "pt_tcp_client_connect: error get sock opts %d, %s", errno, strerror(errno));
                return -1;
            }
            if (fcntl(client_socket, F_SETFL, ret|O_NONBLOCK) < 0) {
                pu_log(LL_ERROR, "pt_tcp_client_connect: error set NONBLOCK sock opts %d, %s", errno, strerror(errno));
                return -1;
            }
            return client_socket;
        }
    }
    return -1;
}
/////////////////////////////////////////////////
//Common part
//
/////////////////////////////////////////////////
//pt_tcp_selector   - wait one of events: READ, WRITE or TIMEOUT
//
//sock - socket from we're waiting one of events
//Return event (see pt_tcp_selector_t in pt_tcp_utl.h) or error.
pt_tcp_selector_t pt_tcp_selector(int sock) {
    int result;
    fd_set readset;
    fd_set writeset;

/* Call select() */
    FD_ZERO(&readset);
    FD_ZERO(&writeset);
    FD_SET(sock, &readset);
    FD_SET(sock, &writeset);
    struct timeval tv = {DEFAULT_SOCK_SELECT_TO_SEC, 0}; // timeout for one second
    result = select(sock + 1, &readset, &writeset, NULL, &tv);
    if (result == 0) { // Timeout happens. nothing to read
        return CU_TIMEOUT;
    }
    if (result < 0) {
        pu_log(LL_ERROR, "pt_tcp_selector: error on select %d, %s", errno, strerror(errno));
        return CU_ERROR;
    }
// We got something to read!
    if(FD_ISSET(sock, &readset)) {
        return CU_READ;
    }
    else if(FD_ISSET(sock, &writeset)) {
        return CU_WRITE;
    }
    return CU_ERROR;
}
/////////////////////////////////////////////////
//pt_tcp_read   - read data from the socket
//
//sock      - socket to read
//buf       - buffer to copy info from the socket
//buf_len   - buf capacity
//Return amount of bytes red; -1 if error
int pt_tcp_read(int sock, char* buf, unsigned buf_len) { // returns >0 if smth to read, 0 if nothing -1 if reconnect needed
    int ret = read(sock, buf, buf_len);
    if (ret > 0) { //Hurray, we read smth!
        return ret;
    } else { // some shit... go to reconnect
        pu_log(LL_ERROR, "pt_tcp_read: error on read %d, %s", errno, strerror(errno));
        return -1;
    }
}
/////////////////////////////////////////////////
//pt_tcp_write   - write data to the socket
//
//sock      - socket to write
//buf       - buffer to copy info to the socket
//buf_len   - message len
//Return amount of bytes written; -1 if error
int pt_tcp_write(int sock, const char* buf, unsigned buf_len) {  // returns >0 if smth to write, 0 if nothing -1 reconnect needed
    int ret = write(sock, buf, buf_len);
    if (ret > 0) { //Hurray, we write smth!
        return ret;
    } else { // some shit... go to reconnect
        pu_log(LL_ERROR, "pt_tcp_write: error on write %d, %s", errno, strerror(errno));
        return -1;
    }
}
/////////////////////////////////////////////////
//pt_tcp_shutdown   - close the connection
//
//sock      - socket to close
void pt_tcp_shutdown(int sock) {
    if(sock >= 0) {
        shutdown(sock, SHUT_RDWR);
        close(sock);
    }
}
/////////////////////////////////////////////////
//pt_tcp_assemble   - assembling message from several tcp parsels if for some reasons the message
//                  wasn't sent as one piece. The sign of end message is the 0-byte
//
//in    - message received
//len   - message length
//ab    - assembling bufer
//Return the 0-terminated message
const char* pt_tcp_assemble(const char* in, unsigned len, pt_tcp_assembling_buf_t* ab) { // assimbling the full message. Return NULL if nothing oe msg
    unsigned i;
    char* ret = NULL;
    assert(in); assert(ab);

    if((ab->idx + len) > ab->buf_len) {
        len = ab->buf_len - ab->idx - 1;
        pu_log(LL_ERROR, "pt_tcp_assemble: assembling buffer overflow! Data truncated\n");
        memcpy(ab->buf+ab->idx, in, len);
        ab->buf[ab->idx + len-1] = '\0';
    }
    else {
        memcpy(ab->buf + ab->idx, in, len);
    }

    ab->idx += len;

    for(i = 0; i < ab->idx; i++) {
        if(ab->buf[i] == '\0') {
            memcpy(ab->retbuf, ab->buf, i+1);
            ret = ab->retbuf;
            memmove(ab->buf, ab->buf+i+1, ab->idx-(i+1));
            ab->idx = ab->idx-(i+1);
            break;
        }
    }
    return ret;
}
