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

NB-1! dependant on pu_logger library
NB-2! no thread protection
*/

#ifndef PRESTO_LIB_HTTP_H
#define PRESTO_LIB_HTTP_H

#include <stdlib.h>
#include <stdio.h>

#define LIB_HTTP_DEVICE_ID_PREFFIX          "aioxGW-"

#define LIB_HTTP_MAX_POST_RETRIES           3
#define LIB_HTTP_MAX_FGET_RETRIES           5
#define LIB_HTTP_HEADER_SIZE                200
#define LIB_HTTP_MAX_MSG_SIZE               8193    /* +1 byte for '\0' */
#define LIB_HTTP_MAX_URL_SIZE               4097
#define LIB_HTTP_AUTHENTICATION_STRING_SIZE 129
#define LIB_HTTP_DEVICE_ID_SIZE             31
#define LIB_HTTP_FW_VERSION_SIZE            129
#define LIB_HTTP_SHA_256_SIZE               64      /* bytes */
/* Long GET from Proxy's side will be less on LIB_HTTP_PROXY_SERVER_TO_DELTA than the cloud conn timeout */
#define LIB_HTTP_PROXY_SERVER_TO_DELTA      20
/* Maximum time for an HTTP connection, including name resolving */
#define LIB_HTTP_DEFAULT_CONNECT_TIMEOUT_SEC    30
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
typedef enum {LIB_HTTP_CONN_INIT_MAIN, LIB_HTTP_CONN_POST, LIB_HTTP_CONN_GET, LIB_HTTP_FILE_GET
} lib_http_conn_type_t;

/* POST return types for upper level analysis */
typedef enum {LIB_HTTP_POST_ERROR = -1, LIB_HTTP_POST_RETRY = 0, LIB_HTTP_POST_OK = 1
} lib_http_post_result_t;

/* http connection handler type */
typedef int lib_http_conn_t;

/***************************************************
 * Create connections pool & initiates curl library
 * @param max_conns_amount  - max amount of simultaneous hppt(s) connections
 * @return  - if OK, 0 if not
 */
int lib_http_init(unsigned int max_conns_amount);

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
void lib_http_eraseConn(lib_http_conn_t conn);

/***************************************************
 * Perform "long GET" request
 * @param get_conn      - connection handler LIB_HTTP_CONN_GET or LIB_HTTP_CONN_INIT_MAIN type
 * @param msg           - buffer for received message
 * @param msg_size      - buffer size
 * @return  - 1 if get msg, 0 if timeout, -1 if error. Logged inside
 */
int lib_http_get(lib_http_conn_t get_conn, char* msg, size_t msg_size);

/***************************************************
 * Perform POST request
 * @param post_conn     - connection handler
 * @param msg           - char string to send
 * @param reply         - buffer for possible reply
 * @param reply_size    - buffer size
 * @param auth_token    - gateway authentication token
 * @return  - 1 if OK, 0 if timeout, -1 if error. if strlen(relpy)>0 there is some answer from the server. All logging inside
 */
lib_http_post_result_t lib_http_post(lib_http_conn_t post_conn, const char* msg, char* reply, size_t reply_size, const char* auth_token);

/***************************************************
 * Reag file
 * @param gf_conn   - connection handler LIB_HTTP_FILE_GET type
 * @param rx_file   - open file descriptor ("wb" mode expected)
 * @return  - 1 if OK, 0 if timeout, -1 if error.
 */
int lib_http_get_file(lib_http_conn_t gf_conn, FILE* rx_file);

#endif /* PRESTO_LIB_HTTP_H */