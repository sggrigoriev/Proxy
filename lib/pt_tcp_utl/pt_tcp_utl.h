//
// Created by gsg on 08/11/16.
//

#ifndef PT_TCP_UTL_H
#define PT_TCP_UTL_H

#define PT_BINDING_ATTEMPTS     1000
#define PT_SOCK_SELECT_TO_SEC   1       //Select timeout in seconds

//Server part
int pt_tcp_server_connect(int prort, int socket, int reconnect);    //Return connected socket (>0), -1 if error
//////////////////////////////////////////////////
//Client part
int pt_tcp_client_connect(int port, int socket);                    //Return connected socket (>0), -1 if error
//////////////////////////////////////////////////
//Common part
typedef enum {CU_READ, CU_WRITE, CU_TIMEOUT, CU_ERROR} pt_tcp_selector_t;

pt_tcp_selector_t pt_tcp_selector(int socket);                      //Return event (see pt_tcp_selector_t in pt_tcp_utl.h) or error.
int pt_tcp_read(int socket, char* buf, unsigned buf_len);           //Return amount of bytes red; -1 if error
int pt_tcp_write(int socket, const char* buf, unsigned buf_len);    //Return amount of bytes written; -1 if error
void pt_tcp_shutdown(int sock);
//

typedef struct{
    char* buf;
    unsigned buf_len;
    unsigned idx;
    char* retbuf;
    unsigned retbuf_len;

} pt_tcp_assembling_buf_t;

const char* pt_tcp_assemble(const char* in, unsigned len, pt_tcp_assembling_buf_t* assempling_buf); //Return the 0-terminated message
#endif //PT_TCP_UTL_H
