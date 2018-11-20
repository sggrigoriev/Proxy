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
 *
 Created by gsg on 19/12/16.
*/

#include <assert.h>
#include <malloc.h>
#include <string.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <sys/select.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>

#include "pu_logger.h"
#include "lib_tcp.h"

/*Thread protection to guard connections pool */
static pthread_mutex_t own_mutex = PTHREAD_MUTEX_INITIALIZER;

/*Add data to assembling bufer
  in  - buffer with data
  ab  - assimbling buffer
Returb 1 if OK; 0 if no more place in assembling buffer
*/
static int tcp_get(lib_tcp_in_t* in, lib_tcp_assembling_buf_t* ab);

/*Close the active connection. Pool will have 1+ connections available
  all_conns   - connections pool
  conn        - the connection to be closed
*/
static void remove_conn(lib_tcp_conn_t* all_conns, lib_tcp_rd_t* conn);

/*Round Robin for checking ready for read connections
  no      - previous start number
  size    - active connections amount
Return next number to start from
*/
static unsigned int inc_start_no(unsigned int no, unsigned int size);

/*Initiate fd_setcalling FD_ZERO for all active connections
  all_conns   - connection pool
  fds         - fdset for all active connections
Return 1 if OK; 0 if error
*/
static int make_fdset(lib_tcp_conn_t* all_conns, fd_set* fds);

/*Get first ready for read connection from all ready connections
  all_conns   - connections pool
  fds         - fdset for all active connections
Return ready connection or NULL
*/
static lib_tcp_rd_t* get_ready_conn(lib_tcp_conn_t* all_conns, fd_set* fds);
/*************************
 * Provides connection human readable name or connection index if the name is not set
 * @param conn  - pointer to connection description
 * @return      - connection name (if set) or connection number
 */
const char* lib_tcp_get_conn_name(const lib_tcp_rd_t* conn) {
    return (!conn)?"":conn->name;
}
/*
Public functions. Description in *h file
*/
lib_tcp_conn_t* lib_tcp_init_conns(unsigned int max_connections, size_t in_size) {
    pthread_mutex_lock(&own_mutex);
    assert(max_connections);
    lib_tcp_conn_t* ret = calloc(sizeof(lib_tcp_conn_t), 1);
    if(!ret) goto on_err;
    ret->rd_conn_array = calloc(max_connections* sizeof(lib_tcp_rd_t), 1);
    if(!ret->rd_conn_array) goto on_err;
    unsigned int i;
    for(i = 0; i < max_connections; i++) {
        ret->rd_conn_array[i].socket = -1;
        ret->rd_conn_array[i].in_buf.buf = calloc(in_size, 1);
        ret->rd_conn_array[i].in_buf.len = 0;
        ret->rd_conn_array[i].in_buf.size = in_size;
        ret->rd_conn_array[i].ass_buf.buf = calloc(in_size*2, 1);
        ret->rd_conn_array[i].ass_buf.size = in_size*2;
        ret->rd_conn_array[i].ass_buf.idx = 0;
        ret->rd_conn_array[i].ass_buf.status = LIB_TCP_BUF_READY;
        if(!ret->rd_conn_array[i].ass_buf.buf || !ret->rd_conn_array[i].in_buf.buf) goto on_err;
        snprintf(ret->rd_conn_array[i].name, sizeof(ret->rd_conn_array[i].name)-1, "%d", i);
    }
    ret->sa_max_size = max_connections;
    ret->sa_size = 0;
    ret->start_no = 0;
    pthread_mutex_unlock(&own_mutex);
    return ret;
on_err:         /* TODO possibly not all the memory frees: ret, in_buf, as_buf */
    pthread_mutex_unlock(&own_mutex);
    return NULL;
}

lib_tcp_rd_t* lib_tcp_add_new_conn(int rd_socket, lib_tcp_conn_t* all_conns) {
    if(!all_conns) {
        pu_log(LL_ERROR, "%s: Connection pool is not initialized");
        return NULL;
    }
    pthread_mutex_lock(&own_mutex);
    unsigned int i;
    for(i = 0; i < all_conns->sa_max_size; i++) {
        if(all_conns->rd_conn_array[i].socket < 0) {
            all_conns->rd_conn_array[i].socket = rd_socket;
            all_conns->sa_size++;
            pthread_mutex_unlock(&own_mutex);
            return all_conns->rd_conn_array+i;
        }
    }
    pthread_mutex_unlock(&own_mutex);
    pu_log(LL_ERROR, "%s: No free slot for new connection", __FUNCTION__);
    return NULL;
}

int lib_tcp_conn_amount(lib_tcp_conn_t* all_conns) {
    return (all_conns)?all_conns->sa_size:0;
}

void lib_tcp_destroy_conns(lib_tcp_conn_t* all_conns) {
    if(!all_conns) return;
    pthread_mutex_lock(&own_mutex);
    unsigned int i;
    for(i = 0; i < all_conns->sa_max_size; i++) {
        if(all_conns->rd_conn_array[i].socket >= 0) {
            shutdown(all_conns->rd_conn_array[i].socket, SHUT_RDWR);
            close(all_conns->rd_conn_array[i].socket);
            free(all_conns->rd_conn_array[i].in_buf.buf);
            free(all_conns->rd_conn_array[i].ass_buf.buf);
        }
    }
    free(all_conns->rd_conn_array);
    free(all_conns);
    pthread_mutex_unlock(&own_mutex);
}

/*All three return -1 if error, 0 if timeout, >0 if value
return binded socket
*/
int lib_tcp_get_server_socket(int port) {
    /*Create server socket */
    int server_socket;

    if (server_socket = socket(AF_INET, SOCK_STREAM|SOCK_NONBLOCK, 0), server_socket  < 0) {
        pu_log(LL_ERROR, "%s: Error socket creation: %d - %s", __FUNCTION__, errno, strerror(errno));
        return -1;
    }
    /*Set socket options */
    int32_t on = 1;
    /*use the socket even if the address is busy (by previously killed process for ex) */
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &on, sizeof (on)) < 0) {
        pu_log(LL_ERROR, "%s: Error setsockopt: %d - %s", __FUNCTION__, errno, strerror(errno));
        return -1;
    }
    /*Make address */
    struct sockaddr_in addr_struct;
    memset(&addr_struct, 0, sizeof(addr_struct));
    addr_struct.sin_family = AF_INET;
    addr_struct.sin_addr.s_addr = INADDR_ANY;
    addr_struct.sin_port = htons(port);

    /*bind socket on repeated mode */
    int rpt = LIB_TCP_BINGING_ATTEMPTS;
    while (bind(server_socket, (struct sockaddr *) &addr_struct, sizeof(addr_struct)) < 0 ) {
        sleep(1);
        if (!rpt--) {
            pu_log(LL_ERROR, "%s: Error binding: %d - %s", __FUNCTION__, errno, strerror(errno));
            return -1;
        }
    }
    return server_socket;
}

/*return connected socket for R/W operations, 0 if timeout and negative if error */
int lib_tcp_listen(int server_socket, int to_sec) {
    /*Make communication sockets
    listen for incoming connection
    */
    if (listen(server_socket, 1) < 0) {
        pu_log(LL_ERROR, "%s: Error listen server socket: %d - %s", __FUNCTION__, errno, strerror(errno));
        return -1;
    }
/* Call select() */
    fd_set readset;
    FD_ZERO(&readset);
    FD_SET(server_socket, &readset);
    struct timeval tv = {to_sec, 0};

    int result = select(server_socket + 1, &readset, NULL, NULL, &tv);
    if(result <= 0) {
        if(result < 0) pu_log(LL_ERROR, "%s: Error select: %d - %s", __FUNCTION__, errno, strerror(errno));
        return result; /* Timeout or error. nothing to read */
    }
/* We got something to read!
accept incoming connection
*/
    struct sockaddr_in cli_addr = {0};
    socklen_t size = sizeof(struct sockaddr);
    int conn_sock = accept(server_socket, (struct sockaddr *) &cli_addr, &size);
    if (conn_sock < 0) {
        pu_log(LL_ERROR, "%s: Error accept: %d - %s", __FUNCTION__, errno, strerror(errno));
        return -1;
    }

    int sock_flags = fcntl(conn_sock, F_GETFL);
    if (sock_flags < 0) {
        pu_log(LL_ERROR, "%s: Error fcntl get flags: %d - %s", __FUNCTION__, errno, strerror(errno));
        return -1;
    }
    if (fcntl(conn_sock, F_SETFL, sock_flags|O_NONBLOCK) < 0) {
        pu_log(LL_ERROR, "%s: Error fcntl set nonblock mode: %d - %s", __FUNCTION__, errno, strerror(errno));
        return -1;
    }
    return conn_sock;
}

/*return connection desriptor or NULL if error or TO. in_buf.len == 0 in case of timeout
 * rc == 0 if TO, -1 if error. All diagnostics are inside
 * */
lib_tcp_rd_t* lib_tcp_read(lib_tcp_conn_t* all_conns, int to_sec, int* rc) {
    assert(all_conns);
/*Build set for select */
    struct timeval tv = {to_sec, 0};
    fd_set readset;


    pthread_mutex_lock(&own_mutex);
    int max_fd = make_fdset(all_conns, &readset);
    pthread_mutex_unlock(&own_mutex);

    if(max_fd < 0) {
        pu_log(LL_ERROR, "%s: Invalid socket returned: %d - %s", __FUNCTION__, errno, strerror(errno));
        *rc = -1;
        return NULL;
    }
    int result = select(max_fd + 1, &readset, NULL, NULL, &tv);
    if(result < 0) {
        pu_log(LL_ERROR, "%s: Error select: %d - %s", __FUNCTION__, errno, strerror(errno));
        *rc = -1;
        return NULL; /* Error. nothing to read */
    }
    if(result == 0) {
        *rc = 0;
        return NULL;   /*timeout */
    }

    pthread_mutex_lock(&own_mutex);
    lib_tcp_rd_t* conn = get_ready_conn(all_conns, &readset);
    pthread_mutex_unlock(&own_mutex);

    if(!conn) {
        pu_log(LL_ERROR, "%s: No ready connections found", __FUNCTION__);
        *rc = LIB_TCP_READ_NO_READY_CONNS;  /*no ready sockets error - they sould be! */
        return NULL;
    }

    conn->in_buf.len = read(conn->socket, conn->in_buf.buf, conn->in_buf.size);  /*todo mlevitin */
    if(conn->in_buf.len <= 0) {         /*Get read error or (0) connection lost- reconnect required */
        if(conn->in_buf.len < 0) {
            pu_log(LL_ERROR, "%s: Error connection # %s read: %d - %s", __FUNCTION__, conn->name, errno, strerror(errno));
            *rc = LIB_TCP_READ_ERROR;
        }
        else {  /* == 0 */
            *rc = LIB_TCP_READ_EOF;
            pu_log(LL_ERROR, "%s: Connesction # %s lost!", __FUNCTION__, conn->name);
        }

        pthread_mutex_lock(&own_mutex);
        remove_conn(all_conns, conn);       /* Connection removed */
        pthread_mutex_unlock(&own_mutex);

        return NULL;
    }

/*Put incoming message into assembling buffer */
    pthread_mutex_lock(&own_mutex);
    int ret = tcp_get(&conn->in_buf, &conn->ass_buf);
    pthread_mutex_unlock(&own_mutex);

    if(!ret) {
        pu_log(LL_ERROR, "%s: incoming message from conn# %s is too long. Ignored", __FUNCTION__, conn->name);
        *rc = LIB_TCP_READ_MSG_TOO_LONG; /*too long record */
        return conn;
    }
    *rc = LIB_TCP_READ_OK;
    return conn;
}

/*Return the 0-terminated message of NULL if no finished string */
const char* lib_tcp_assemble(lib_tcp_rd_t* conn, char* out, size_t out_size) {
    unsigned i;
    assert(conn);
    assert(out);
    assert(out_size);
    const char* ret = NULL;

    pthread_mutex_lock(&own_mutex);
    for(i = 0; i < conn->ass_buf.idx; i++) {
        if(conn->ass_buf.buf[i] == '\0') {  /* Message tail found */
            if(conn->ass_buf.status != LIB_TCP_BUF_REJECT) {    /* Message will be passed */
                if ((i + 1) <= out_size) {     /*The message fits into out buffer */
                    memcpy(out, conn->ass_buf.buf, i + 1);
                    ret = out;
                } else {
                    pu_log(LL_ERROR, "lib_tcp_assemble: incoming message exceeds max bufer size: %d vs %d. Ignored.", i+1, out_size);
                }
            }
            else {                      /* Reject status - ignore the message */
                pu_log(LL_ERROR, "lib_tcp_assemble: too long incoming message tail found. Ignored");
            }
            conn->ass_buf.status = LIB_TCP_BUF_READY;   /* Reset the status */

            memmove(conn->ass_buf.buf, conn->ass_buf.buf+i+1, conn->ass_buf.idx-(i+1));
            conn->ass_buf.idx = conn->ass_buf.idx-(i+1);
            break;
        }
    }
    pthread_mutex_unlock(&own_mutex);
    return ret;
}

/*All three return -1 if error, 0 if timeout, >0 if value
return binded socket
*/
int lib_tcp_get_client_socket(int port, int to_sec) {
    int client_socket;
/* Create socket */
    if (client_socket = socket(AF_INET, SOCK_STREAM, 0), client_socket < 0) {
        pu_log(LL_ERROR, "%s: Error socket: %d - %s", __FUNCTION__, errno, strerror(errno));
        return -1;
    }

    /*Set socket options */
    int32_t on = 1;
    /*use the socket even if the address is busy (by previously killed process for ex) */
    if (setsockopt(client_socket, SOL_SOCKET, SO_REUSEADDR, &on, sizeof (on)) < 0) {
        pu_log(LL_ERROR, "%s: Error setsockopt: %d - %s", __FUNCTION__, errno, strerror(errno));
        return -1;
    }

/*Make address */
    struct sockaddr_in addr_struct;
    memset(&addr_struct, 0, sizeof(addr_struct));
    addr_struct.sin_family = AF_INET;
    addr_struct.sin_addr.s_addr = INADDR_ANY;
    addr_struct.sin_port = htons(port);

/*And connect to the remote socket */
    unsigned rpt = LIB_TCP_BINGING_ATTEMPTS;
    while(rpt) {
        int ret = connect(client_socket, (struct sockaddr *)&addr_struct, sizeof(addr_struct));
        if (ret < 0) {
            rpt--;
            sleep(1);   /*wait for a while */
        }
        else {
            int sock_flags = fcntl(client_socket, F_GETFL);
            if (sock_flags < 0) {
                pu_log(LL_ERROR, "%s: Error fcntl get flags: %d - %s", __FUNCTION__, errno, strerror(errno));
                return -1;
            }
            if (fcntl(client_socket, F_SETFL, sock_flags|O_NONBLOCK) < 0) {
                pu_log(LL_ERROR, "%s: Error fcntl set nonblock: %d - %s", __FUNCTION__, errno, strerror(errno));
                return -1;
            }
            return client_socket;
        }
    }
    return -1;
}

/*Return bytes sent amt, 0 if timeout, < 0 if error
mlevitin ssize_t lib_tcp_write(int wr_socket, const char* out, size_t size, int to_sec) {
*/
int lib_tcp_write(int wr_socket, const char* out, size_t size, int to_sec) {
    struct timeval tv = {to_sec, 0};
    fd_set writedset;
    FD_ZERO (&writedset);
    FD_SET(wr_socket, &writedset);

    int result = select(wr_socket + 1, NULL, &writedset, NULL, &tv);
    if(result < 0) {
        pu_log(LL_ERROR, "%s: Error select: %d - %s", __FUNCTION__, errno, strerror(errno));
        return -1; /* Error. nothing to write */
    }
    if(result == 0) return 0;   /*timeout */

/*We got just one socket awaiting, so no need to check which is set */
    int ret = write(wr_socket, out, size);
    if(ret < 0) {
        pu_log(LL_ERROR, "%s: Error write: %d - %s", __FUNCTION__, errno, strerror(errno));
    }
    return ret;
}

void lib_tcp_client_close(int write_socket) {
    if(write_socket >= 0) {
        shutdown(write_socket, SHUT_RDWR);
        close(write_socket);
    }
}

const char* lib_tcp_local_ip(const char* interface, char* ip_address, size_t size) {
    struct ifaddrs* addrs;
    struct ifaddrs* tmp;

    ip_address[0] = '\0';
    if((!interface) || (interface[0] == '\0')) {
        return ip_address;
    }

    getifaddrs(&addrs);
    tmp = addrs;

    while (tmp) {
        if (tmp->ifa_addr && tmp->ifa_addr->sa_family == AF_INET) {
            struct sockaddr_in *pAddr = (struct sockaddr_in *)tmp->ifa_addr;
            if(!strcmp(tmp->ifa_name, interface)) { /* Found the interface! */
                strncpy(ip_address, inet_ntoa(pAddr->sin_addr), size-1);
                ip_address[size-1] = '\0';
                break;
            }
        }

        tmp = tmp->ifa_next;
    }

    freeifaddrs(addrs);
    return ip_address;
}

/*
Local functions implementation (Description on top)
*/
static void remove_conn(lib_tcp_conn_t* all_conns, lib_tcp_rd_t* conn) {
    if(!conn || conn->socket < 0) return;
    shutdown(conn->socket, SHUT_RDWR);
    close(conn->socket);
    conn->socket = -1;
    conn->ass_buf.idx = 0;
    conn->in_buf.len = 0;
    all_conns->sa_size--;
}

static unsigned int inc_start_no(unsigned int no, unsigned int size) {
    return (++no < size)?no:0;
}

static int tcp_get(lib_tcp_in_t* in, lib_tcp_assembling_buf_t* ab) {
    if((ab->idx >= ab->size) || ((ab->size - ab->idx) < in->len)) {     /*no place in buffer */
        ab->idx = 0;    /*reset assembling buffer!*/
        ab->status = LIB_TCP_BUF_REJECT;
        return 0;
    }

    memcpy(ab->buf+ab->idx, in->buf, in->len);
    ab->idx += in->len;

    return 1;
}

static int make_fdset(lib_tcp_conn_t* all_conns, fd_set* fds) {
    int max_fd = -1;
    FD_ZERO(fds);
    unsigned int i;
    for(i = 0; i < all_conns->sa_max_size; i++) { /* get all valid sockets */
        if(all_conns->rd_conn_array[i].socket > 0) FD_SET(all_conns->rd_conn_array[i].socket, fds);
        if(all_conns->rd_conn_array[i].socket > max_fd) max_fd = all_conns->rd_conn_array[i].socket;
    }
    return max_fd;
}

static lib_tcp_rd_t* get_ready_conn(lib_tcp_conn_t* all_conns, fd_set* fds) {
    unsigned int idx = all_conns->start_no;
    lib_tcp_rd_t* ret = NULL;
    unsigned int i;
    for(i = 0; (i < all_conns->sa_max_size)&&(!ret); i++) {
        if((all_conns->rd_conn_array[idx].socket >= 0) && FD_ISSET(all_conns->rd_conn_array[idx].socket, fds)) ret = all_conns->rd_conn_array+idx;
        idx = inc_start_no(idx, all_conns->sa_max_size);
    }
    all_conns->start_no = inc_start_no(all_conns->start_no, all_conns->sa_max_size);
    return ret;
}