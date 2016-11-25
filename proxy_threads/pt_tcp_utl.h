//
// Created by gsg on 08/11/16.
//

#ifndef PT_TCP_UTL_H
#define PT_TCP_UTL_H

//Server part
int pt_tcp_server_connect(int prort, int socket, int reconnect);  // returns socket or -1 if error
//////////////////////////////////////////////////
//Client part
int pt_tcp_client_connect(int port, int socket);  // returns socket or -1 if error
//////////////////////////////////////////////////
//Common part
typedef enum {CU_READ, CU_WRITE, CU_TIMEOUT, CU_ERROR} pt_tcp_selector_t;
pt_tcp_selector_t pt_tcp_selector(int socket);
int pt_tcp_read(int socket, char* buf, unsigned buf_len); // returns >0 if smth to read, 0 if nothing -1 if reconnect needed
int pt_tcp_write(int socket, const char* buf, unsigned buf_len);  // returns >0 if smth to write, 0 if nothing -1 reconnect needed
void pt_tcp_shutdown(int sock);
//

typedef struct{
    char* buf;
    unsigned buf_len;
    unsigned idx;
    char* retbuf;
    unsigned retbuf_len;

} pt_tcp_assembling_buf_t;

const char* pt_tcp_assemble(const char* in, unsigned len, pt_tcp_assembling_buf_t* assempling_buf); // assimbling the full message. Return NULL if nothing oe msg
#endif //PT_TCP_UTL_H
