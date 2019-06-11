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
    Created by gsg on 15/12/16.

  Wrapper under cURL for gateway communication with the cloud via http(s) protocol

NB-1! dependent on pu_logger library
NB-2! no thread protection
*/

#ifndef PRESTO_LIB_HTTP_H
#define PRESTO_LIB_HTTP_H

#include <stdlib.h>
#include <stdio.h>

#define LIB_HTTP_DEVICE_ID_PREFFIX          "aioxGW-"

#define LIB_HTTP_MAX_IPADDRES_SIZE          46  /*45 for IPv6 +1 */
#define LIB_HTTP_MAX_IP_INTERFACE_SIZE      129 /* its smaller than 128 bytes for sure! */
#define LIB_HTTP_MAX_POST_RETRIES           3
#define LIB_HTTP_MAX_FGET_RETRIES           5
#define LIB_HTTP_HEADER_SIZE                200
#define LIB_HTTP_MAX_MSG_SIZE               16385    /* +1 byte for '\0' */
#define LIB_HTTP_MAX_URL_SIZE               4097
#define LIB_HTTP_AUTHENTICATION_STRING_SIZE 129
#define LIB_HTTP_DEVICE_ID_SIZE             32
#define LIB_HTTP_FW_VERSION_SIZE            129
#define LIB_HTTP_SHA_256_SIZE               64      /* bytes */
/* Long GET from Proxy's side will be less on LIB_HTTP_PROXY_SERVER_TO_DELTA than the cloud conn timeout */
#define LIB_HTTP_PROXY_SERVER_TO_DELTA      20
/* Maximum time for an HTTP connection, including name resolving */
#define LIB_HTTP_DEFAULT_CONNECT_TIMEOUT_SEC    300
/* Maximum time for an HTTP transfer */
#define LIB_HTTP_DEFAULT_TRANSFER_TIMEOUT_SEC   60


#define LIB_HTTP_MAIN_CONN_IFACE    "/cloud/json/settingsServer?type=deviceio&deviceId="
#define LIB_HTTP_ROUTINE_CONN_IFACE "/deviceio/mljson?id="

/*
Connection types:
    LIB_HTTP_CONN_INIT_MAIN   - main connection to cloud (provides the connection for common work)
    LIB_HTTP_CONN_POST        - connection to make POSTs
    LIB_HTTP_CONN_GET         - connection to make GETs
    LIB_HTTP_FILE_GET         -- connection for file upload
*/
typedef enum {LIB_HTTP_CONN_INIT_MAIN, LIB_HTTP_CONN_INIT_MAIN_NOSSL, LIB_HTTP_CONN_POST, LIB_HTTP_CONN_GET, LIB_HTTP_FILE_GET
} lib_http_conn_type_t;

/* POST return types for upper level analysis */
/* Result of cloud JSON answer parsing:
 * LIB_HTTP_IO_ERROR        - RC < 0 or error parsing cloud JSON or unsupported cloud answer
 * LIB_HTTP_IO_RETRY        - RC = 0 or cloud answers "ERR" or cURL returns EAGAIN or ETIMEDOUT
 * LIB_HTTP_IO_OK
 * LIB_HTTP_IO_UNKNOWN      - cloud returns "UNKNOWN" - unregistered device
 * LIB_HTTP_IO_UNAUTH       - cloud returns "UNAUTHORIZED" - wrong auth_token was sent by Proxy
 *
 */
typedef enum {LIB_HTTP_IO_ERROR = -1, LIB_HTTP_IO_RETRY = 0, LIB_HTTP_IO_OK = 1, LIB_HTTP_IO_UNKNOWN = 2, LIB_HTTP_IO_UNAUTH = 3
} lib_http_io_result_t;

/* http connection handler type */
typedef int lib_http_conn_t;

/***************************************************
 * Create connections pool & initiates curl library
 * @param max_conns_amount  - max amount of simultaneous hppt(s) connections
 * @param sslverify         - see the "CURLOPT_SSL_VERIFYPEER" setting in *.conf files
 * @param cainfo            - see the "CURLOPT_CAINFO" setting in *conf files
 * @return  - if OK, 0 if not
 */
int lib_http_init(unsigned int max_conns_amount, int sslverify, const char* cainfo);

/***************************************************
 * Clean-up connection pool; close curl library
 */
void lib_http_close();

/***************************************************
 * Create the connection of conn_type.
 * @param conn_type     - created connection type.
 * @param url           - url string
 * @param auth_token    - the gateway auth token for authorization on cloud side
 * @param deviceID      - the gateway device id
 * @param conn_to       - connection timeout in seconds
 * @return  - connection handler or -1 if error
 */
lib_http_conn_t lib_http_createConn(lib_http_conn_type_t conn_type, const char *url, const char* auth_token, const char* deviceID, unsigned int conn_to);

/***************************************************
 * Erase the connection. Free the place in connection pool
 * @param conn  - connection handler
 */
void lib_http_eraseConn(lib_http_conn_t* conn);

/***************************************************
 * Perform "long GET" request
 * @param get_conn          - connection handler LIB_HTTP_CONN_GET or LIB_HTTP_CONN_INIT_MAIN type
 * @param answers           - list of commandsId from commands received during previous call or "" if were no commands
 * @param msg               - buffer for received message
 * @param msg_size          - buffer size
 * @param no_json           - 0 if no JSON expected in answer
 * @param keepalive_interval- period in seconds of keepalive sendings
 * @return  - see lib_http_io_result_t; Logged inside
 */
lib_http_io_result_t lib_http_get(lib_http_conn_t get_conn, const char* answers, char* msg, size_t msg_size, int no_json, unsigned long keepalive_interval);

/***************************************************
 * Perform POST request
 * @param post_conn     - connection handler
 * @param msg           - char string to send
 * @param reply         - buffer for possible reply
 * @param reply_size    - buffer size
 * @param auth_token    - gateway authentication token
 * @return  - see lib_http_io_result_t; Logged inside
 */
lib_http_io_result_t lib_http_post(lib_http_conn_t post_conn, const char* msg, char* reply, size_t reply_size, const char* auth_token);

/***************************************************
 * Read file
 * @param gf_conn   - connection handler LIB_HTTP_FILE_GET type
 * @param rx_file   - open file descriptor ("wb" mode expected)
 * @return  - 1 if OK, 0 if timeout, -1 if error.
 */
int lib_http_get_file(lib_http_conn_t gf_conn, unsigned long keepalive_interval, FILE* rx_file);

/**
 * @brief   Called when a message has to be sent to the server. this is a standard streamer
 *              if the size of the data to write is larger than size*nmemb, this function
 *              will be called several times by libcurl.
 *
 * @param ptr     - where data has to be written
 * @param size    - size*nmemb == maximum number of bytes that can be written each time
 * @param nmemb   - size*nmemb == maximum number of bytes that can be written each time
 * @param userp   - ptr to message to write -> inputted by CURLOPT_READDATA call below *
 * @return  - number of bytes that were written
 **/
size_t read_callback(void *ptr, size_t size, size_t nmemb, void *userp);

#endif /* PRESTO_LIB_HTTP_H */