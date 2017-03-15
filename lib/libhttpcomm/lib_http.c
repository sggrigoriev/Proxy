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
*/
#include <errno.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <curl/curl.h>
#include <assert.h>

#include "cJSON.h"
#include "pu_logger.h"
#include "lib_http.h"

/* Uncomment or add the "add_definitions( -DLIBHTTP_CURL_DEBUG)" string in /Proxy/CmakeLists.txt if debug output needed */
#ifdef LIBHTTP_CURL_DEBUG
struct data {
  char trace_ascii; /* 1 or 0 */
};
static int my_trace(CURL *handle, curl_infotype type, char *data, size_t size, void *userp);
#endif

/* structure used to store data to be sent to the server. */
typedef struct {
    char * buffer;
    int size;
} HttpIoInfo_t;

/*
HTTP connection handler body definition
  type            - connection type
  hndlr           - cURL connection handler
  url             - URL to be connected with
  inBoundCommInfo - data transferred
  rx_buf          - data received
  err_buf         - buffer for error receiving
  slist           - some cURL bullshit
*/
typedef struct {
    lib_http_conn_type_t h_type;
    CURLSH* hndlr;
    char url[LIB_HTTP_MAX_URL_SIZE];
    HttpIoInfo_t inBoundCommInfo;
    char rx_buf[LIB_HTTP_MAX_MSG_SIZE];
    char err_buf[CURL_ERROR_SIZE];
    struct curl_slist* slist;
} http_handler_t;
/**************************************************
 Local data
*/

static http_handler_t** CONN_ARRAY;             /* Connections pool pointer */
static unsigned int CONN_ARRAY_SIZE = 0;        /* Connections pool size */

/******************************************
    Static functions declaration
*/

/******************************************
 * Allocate space for connections pool
 * @param connections_max   - max sumultaneous connections amount
 * @return  - NULL or pointer to the space allocated
 */
static http_handler_t** alloc_conn_pull(unsigned connections_max);

/******************************************
 * Write 0s to all connection fields as initiation
 * @param conn  - pointer to the connection body
 */
static void bzero_conn(http_handler_t* conn);

/******************************************
 * Get not used element from connections pool
 * @param conn_array    - pointer to connection pool
 * @param conn_max      - pool size
 * @return  - free connection index or conn_max if error or no free place
 */
static lib_http_conn_t get_free_conn(http_handler_t** conn_array, unsigned int conn_max);

/******************************************
 * Check the vaidity of connection handler
 * @param conn  - checked handler
 * @return  - connection body or NULL (assertion in debug mode)
 */
static http_handler_t* check_conn(lib_http_conn_t conn);

/******************************************
 *  Calc http_write (POST) result
 * @param result    - answer from the cloud. Checked only if rc = 1
 * @param rc        - received code from POST operation.
 * @return
 *    LIB_HTTP_POST_ERROR if rc == -1 or synrax error in result;
 *    LIB_HTTP_POST_RETRY if rc == 0 or result has status == "ERR" or status has unknown value
 *    LIB_HTTP_POST_OK in the rest of cases
 */
static lib_http_post_result_t calc_post_result(char* result, int rc);

/******************************************
 * Called when a message HAS TO BE SENT to the server.
 * @param ptr   - where data has to be written
 * @param size  - size*nmemb == maximum number of bytes that can be written each time
 * @param nmemb - size*nmemb == maximum number of bytes that can be written each time
 * @param userp - ptr to message to write -> inputted by CURLOPT_READDATA call below
 * @return  - number of bytes that were written
 */
static size_t read_callback(void *ptr, size_t size, size_t nmemb, void *userp);

/******************************************
 * Called when a message HAS TO BE RECEIVED from the server
 * @param ptr   - where the received message resides
 * @param size  - size*nmemb == number of bytes to read
 * @param nmemb - size*nmemb == number of bytes to read
 * @param userp - ptr to where the message will be written -> inputted by CURLOPT_WRITEDATA call below
 * @return  - number of bytes that were written
 */
static size_t writer(void *ptr, size_t size, size_t nmemb, void *userp);

/******************************************
 * Callback for file upload. Writes data from buffer to the file stream
 * @param buffer    - incoming data buffer
 * @param size      - element size in bytes
 * @param nmemb     - amount of elements in the buffer. size*nmemb = buffer size
 * @param stream    -  open ("wb" mode) file stream
 * @return
 */
static long file_writer(void *buffer, size_t size, size_t nmemb, void *stream);

/******************************************
    Public functions implementation
*/

int lib_http_init(unsigned int max_conns_amount) {
    assert(max_conns_amount);

    if((CONN_ARRAY != NULL) || (CONN_ARRAY_SIZE > 0)) {
        pu_log(LL_ERROR, "lib_http_init: connections pool already initiated");
        return 0;
    }
    if(CONN_ARRAY = alloc_conn_pull(max_conns_amount), CONN_ARRAY == NULL) return 0;
    CONN_ARRAY_SIZE = max_conns_amount;
    if (curl_global_init(CURL_GLOBAL_ALL) != 0) {
        pu_log(LL_ERROR, "Error on cUrl initialiation. Something goes wrong. Exiting.");
        return 0;
    }
    return 1;
}

void lib_http_close() {
    unsigned int i;
    for(i = 0; i < CONN_ARRAY_SIZE; i++) {
        if(CONN_ARRAY[i]) lib_http_eraseConn(i);
    }
    free(CONN_ARRAY);
    CONN_ARRAY = NULL;
    CONN_ARRAY_SIZE = 0;

    curl_global_cleanup();
}

lib_http_conn_t lib_http_createConn(lib_http_conn_type_t conn_type, const char *purl, const char* auth_token, const char* deviceID, unsigned int conn_to) {
    lib_http_conn_t conn;

    assert(purl);
    if(conn_type != LIB_HTTP_FILE_GET) {
        assert(auth_token); assert(deviceID);
    }

    if(conn = get_free_conn(CONN_ARRAY, CONN_ARRAY_SIZE), conn >= CONN_ARRAY_SIZE) {
        pu_log(LL_ERROR, "lib_http_createConn: no free slots to create the HTTP connection");
        return -1;
    }
    http_handler_t* handler = CONN_ARRAY[conn];
    handler->h_type = conn_type;
#ifdef LIBHTTP_CURL_DEBUG
    struct data config;
    config.trace_ascii = 1; /* enable ascii tracing */
#endif
    CURLcode curlResult = CURLE_OK;

/* URL string creation */
    strncpy(handler->url, purl, sizeof(handler->url));
    switch(conn_type) {
        case LIB_HTTP_CONN_INIT_MAIN:                       /* Get contact url from the main url */
            strncat(handler->url, LIB_HTTP_MAIN_CONN_IFACE, sizeof(handler->url)-strlen(handler->url)-1);
            break;
        case LIB_HTTP_FILE_GET:                             /*No interface - just the link */
            break;
        default:                                            /* Rest of cases */
            strncat(handler->url, LIB_HTTP_ROUTINE_CONN_IFACE, sizeof(handler->url)-strlen(handler->url)-1);
            break;
    }
    if(conn_type != LIB_HTTP_FILE_GET) {                    //Add device id (all have it)
        strncat(handler->url, deviceID, sizeof(handler->url) - strlen(handler->url) - 1);
    }
    if(conn_type == LIB_HTTP_CONN_GET) {                    //Add timeout for routine GET
        char buf[20];
        snprintf(buf, sizeof(buf), "&timeout=%d", conn_to);
        strncat(handler->url, buf, sizeof(handler->url) - strlen(handler->url) - 1);
        conn_to += LIB_HTTP_PROXY_SERVER_TO_DELTA;
    }

/* Handler initiation */
    if(handler->hndlr = curl_easy_init(), !handler->hndlr) {
        pu_log(LL_ERROR, "lib_http_createConn: cURL handler creation failed.");
        goto out;
    }
/* slist creation for LIB_HTTP_CONN_GET only: slist for POST should be created for each POST call: it depends on posting message size */
    if((conn_type == LIB_HTTP_CONN_GET) || (conn_type == LIB_HTTP_FILE_GET)) {
        handler->slist = curl_slist_append(handler->slist, "User-Agent: IOT Proxy");
        if(conn_type == LIB_HTTP_CONN_GET) {
            char buf[LIB_HTTP_AUTHENTICATION_STRING_SIZE + 50];
            snprintf(buf, sizeof(buf), "PPCAuthorization: esp token=%s", auth_token);
            handler->slist = curl_slist_append(handler->slist, buf);
        }
        if(curlResult = curl_easy_setopt(handler->hndlr, CURLOPT_HTTPHEADER, handler->slist), curlResult != CURLE_OK) goto out;
    }

/* Set options to the handler */
#ifdef LIBHTTP_CURL_DEBUG
    curl_easy_setopt(handler->hndlr, CURLOPT_DEBUGFUNCTION, my_trace);
    curl_easy_setopt(handler->hndlr, CURLOPT_DEBUGDATA, &config);
    /* the DEBUGFUNCTION has no effect until we enable VERBOSE */
    curl_easy_setopt(handler->hndlr, CURLOPT_VERBOSE, 1L);
#endif
    if(curlResult = curl_easy_setopt(handler->hndlr, CURLOPT_NOSIGNAL, 1L), curlResult != CURLE_OK) goto out;
    if(curlResult = curl_easy_setopt(handler->hndlr, CURLOPT_SSLVERSION, CURL_SSLVERSION_TLSv1_2), curlResult != CURLE_OK) goto out;
    if(curlResult = curl_easy_setopt(handler->hndlr, CURLOPT_CONNECTTIMEOUT, conn_to), curlResult != CURLE_OK) goto out;
    if(curlResult = curl_easy_setopt(handler->hndlr, CURLOPT_ERRORBUFFER, handler->err_buf), curlResult != CURLE_OK) goto out;
    if(curlResult = curl_easy_setopt(handler->hndlr, CURLOPT_URL, handler->url), curlResult != CURLE_OK) goto out;
    if(conn_type != LIB_HTTP_FILE_GET) {    //for file get we'll do it inside the get itself
        if (curlResult = curl_easy_setopt(handler->hndlr, CURLOPT_WRITEFUNCTION, writer), curlResult != CURLE_OK) goto out;
        if (curlResult = curl_easy_setopt(handler->hndlr, CURLOPT_WRITEDATA, &handler->inBoundCommInfo), curlResult != CURLE_OK) goto out;
        if(curlResult = curl_easy_setopt(handler->hndlr, CURLOPT_BUFFERSIZE, sizeof(handler->rx_buf)), curlResult != CURLE_OK) goto out;
    }
    switch (conn_type) {
        case LIB_HTTP_CONN_POST:
            if(curlResult = curl_easy_setopt(handler->hndlr, CURLOPT_POST, 1L), curlResult != CURLE_OK) goto out;
            if(curlResult = curl_easy_setopt(handler->hndlr, CURLOPT_POSTFIELDS, 0L), curlResult != CURLE_OK) goto out;
            if(curlResult = curl_easy_setopt(handler->hndlr, CURLOPT_TIMEOUT, LIB_HTTP_DEFAULT_TRANSFER_TIMEOUT_SEC), curlResult != CURLE_OK) goto out;
            break;
        case LIB_HTTP_CONN_GET:
            if(curlResult = curl_easy_setopt(handler->hndlr, CURLOPT_TIMEOUT, conn_to), curlResult != CURLE_OK) goto out;
            if(curlResult = curl_easy_setopt(handler->hndlr, CURLOPT_TCP_KEEPALIVE, 1L), curlResult != CURLE_OK) goto out;
            if(curlResult = curl_easy_setopt(handler->hndlr, CURLOPT_TCP_KEEPIDLE, (long)conn_to+1), curlResult != CURLE_OK) goto out;
            if(curlResult = curl_easy_setopt(handler->hndlr, CURLOPT_TCP_KEEPINTVL, (long)conn_to+1), curlResult != CURLE_OK) goto out;
            break;
        default:
            if(curlResult = curl_easy_setopt(handler->hndlr, CURLOPT_TIMEOUT, conn_to), curlResult != CURLE_OK) goto out;
            break;
    }
out:
    if(curlResult != CURLE_OK) {
        pu_log(LL_ERROR, "lib_http_createConn: %s", curl_easy_strerror(curlResult));
        lib_http_eraseConn(conn);
        return -1;
    }
    return conn;
}

void lib_http_eraseConn(lib_http_conn_t conn) {
    if((conn >= CONN_ARRAY_SIZE) || (!CONN_ARRAY[conn])) return;
    if(CONN_ARRAY[conn]->slist) curl_slist_free_all(CONN_ARRAY[conn]->slist);
    if(CONN_ARRAY[conn]->hndlr) curl_easy_cleanup(CONN_ARRAY[conn]->hndlr);
    free(CONN_ARRAY[conn]);
    CONN_ARRAY[conn] = NULL;
}

int lib_http_get(lib_http_conn_t get_conn, char* msg, size_t msg_size) {
    long httpResponseCode = 0;
    long httpConnectCode = 0;
    long curlErrno = 0;
    http_handler_t* handler = NULL;

    assert(msg);

    if(handler = check_conn(get_conn), handler == NULL) return -1;

    handler->inBoundCommInfo.buffer =  handler->rx_buf;
    handler->inBoundCommInfo.size = sizeof(handler->rx_buf);
    bzero(handler->rx_buf, sizeof(handler->rx_buf));

    msg[0] = '\0';  /* in case we got nothing */

    CURLcode curlResult = curl_easy_perform(handler->hndlr);

    curl_easy_getinfo(handler->hndlr, CURLINFO_RESPONSE_CODE, &httpResponseCode );
    curl_easy_getinfo(handler->hndlr, CURLINFO_HTTP_CONNECTCODE, &httpConnectCode );

    if (httpResponseCode >= 300 || httpConnectCode >= 300) {
        pu_log(LL_ERROR, "lib_http_get: HTTP error response code:%ld, connect code:%ld", httpResponseCode, httpConnectCode);
        curlErrno = EHOSTUNREACH;
        goto out;
    }

    if (curlResult != CURLE_OK) {
        if (curlResult == CURLE_ABORTED_BY_CALLBACK) {
            curlErrno = EAGAIN;
            pu_log(LL_DEBUG, "lib_http_get: quitting curl transfer: %d %s", curlErrno, strerror((int)curlErrno));
        }
        else {
            if (curl_easy_getinfo(handler->hndlr, CURLINFO_OS_ERRNO, &curlErrno) != CURLE_OK) {
                curlErrno = ENOEXEC;
                pu_log(LL_ERROR, "lib_http_get: curl_easy_getinfo");
            }
            if (curlResult == CURLE_OPERATION_TIMEDOUT) curlErrno = ETIMEDOUT; /* time out error must be distinctive */
            else if (curlErrno == 0) curlErrno = ENOEXEC; /* can't be equal to 0 if curlResult != CURLE_OK */

            pu_log(LL_WARNING, "lib_http_get: %s, %s for url %s", curl_easy_strerror(curlResult), strerror((int) curlErrno), handler->url);
        }
        goto out;
    }
    /* the following is a special case - a time-out from the server is going to return a */
    /* string with 1 character in it ... */
    if (strlen(handler->rx_buf) > 1) {
        /* put the result into the main buffer and return */
        pu_log(LL_DEBUG, "lib_http_get: received msg length %d", strlen(handler->rx_buf));
        strncpy(msg, handler->rx_buf, msg_size-1);
    }
    else {
        pu_log(LL_DEBUG, "lib_http_get: received time-out message from the server. RX len = %d", strlen(handler->rx_buf));
        curlErrno = EAGAIN;
        goto out;
    }
    out:
    handler->rx_buf[0] = '\0';    /* prepare rx_buf for new inputs */
    if((curlErrno == EAGAIN) || (curlErrno == ETIMEDOUT)) return 0;   /* timrout case */
    if(!curlErrno) return 1; /* Got smth to read */
    return -1;
}

lib_http_post_result_t lib_http_post(lib_http_conn_t post_conn, const char* msg, char* reply, size_t reply_size, const char* auth_token) {
    CURLcode curlResult;
    http_handler_t* handler = NULL;
    char tx_buf[LIB_HTTP_MAX_MSG_SIZE];
    assert(msg); assert(reply); assert(auth_token);

    if(handler = check_conn(post_conn), handler == NULL) return LIB_HTTP_POST_ERROR;

    HttpIoInfo_t outBoundCommInfo ={tx_buf, (int)(strlen(msg))};
    strncpy(tx_buf, msg, outBoundCommInfo.size+1);

    handler->inBoundCommInfo.buffer =  handler->rx_buf;
    handler->inBoundCommInfo.size = sizeof(handler->rx_buf);
    bzero(handler->rx_buf, sizeof(handler->rx_buf));


/* Create slist */
    if(strlen(msg) > 0) {
        char buf[41 + LIB_HTTP_AUTHENTICATION_STRING_SIZE];
        handler->slist = curl_slist_append(handler->slist, "Content-Type: application/json");
        snprintf(buf, sizeof(buf) - 1, "Content-Length: %d", outBoundCommInfo.size);
        handler->slist = curl_slist_append(handler->slist, buf);

        if (strlen(auth_token) > 0) {
            handler->slist = curl_slist_append(handler->slist, "User-Agent: IOT Proxy");
            snprintf(buf, sizeof(buf) - 1, "PPCAuthorization: esp token=%s", auth_token);
            handler->slist = curl_slist_append(handler->slist, buf);
        }
    }
    if(curlResult = curl_easy_setopt(handler->hndlr, CURLOPT_HTTPHEADER, handler->slist), curlResult != CURLE_OK) goto out;
    if(curlResult = curl_easy_setopt(handler->hndlr, CURLOPT_READFUNCTION, read_callback), curlResult != CURLE_OK) goto out;
    if(curlResult = curl_easy_setopt(handler->hndlr, CURLOPT_READDATA, &outBoundCommInfo), curlResult != CURLE_OK) goto out;

    curlResult = curl_easy_perform(handler->hndlr);

    long httpResponseCode = 0;
    long httpConnectCode = 0;
    long curlErrno = 0;

    curl_easy_getinfo(handler->hndlr, CURLINFO_RESPONSE_CODE, &httpResponseCode );
    curl_easy_getinfo(handler->hndlr, CURLINFO_HTTP_CONNECTCODE, &httpConnectCode );

    if (httpResponseCode >= 300 || httpConnectCode >= 300) {
        pu_log(LL_ERROR, "lib_http_post: HTTP error response code:%ld, connect code:%ld", httpResponseCode, httpConnectCode);
        curlErrno = EHOSTUNREACH;
        goto out;
    }
    if (curlResult != CURLE_OK) {
        if (curlResult == CURLE_ABORTED_BY_CALLBACK) {
            curlErrno = EAGAIN;
            pu_log(LL_DEBUG, "lib_http_post: quitting curl transfer: %d %s", curlErrno, strerror((int)curlErrno));
        }
        else {
            if (curl_easy_getinfo(handler->hndlr, CURLINFO_OS_ERRNO, &curlErrno) != CURLE_OK) {
                curlErrno = ENOEXEC;
                pu_log(LL_ERROR, "lib_http_post: curl_easy_getinfo");
            }
            if (curlResult == CURLE_OPERATION_TIMEDOUT) curlErrno = ETIMEDOUT; /* time out error must be distinctive */
            else if (curlErrno == 0) curlErrno = ENOEXEC; /* can't be equalt to 0 if curlResult != CURLE_OK */

            pu_log(LL_WARNING, "lib_http_post: %s, %s for url %s", curl_easy_strerror(curlResult), strerror((int) curlErrno), handler->url);
        }
        goto out;
    }
    /* the following is a special case - a time-out from the server is going to return a */
    /* string with 1 character in it ... */
    if (strlen(handler->rx_buf) > 0) {
        /* put the result into the main buffer and return */
        pu_log(LL_DEBUG, "lib_http_post: received msg length %d", strlen(handler->rx_buf));
        strncpy(reply, handler->rx_buf, reply_size-1);
    }
    else {
        pu_log(LL_DEBUG, "lib_http_post: received time-out message from the server");
        curlErrno = EAGAIN;
        goto out;
    }
    out:
    {
        int ret;
        if (handler->slist) { curl_slist_free_all(handler->slist); handler->slist = NULL;}

        if (curlErrno == EAGAIN) ret = 0;   /* timeout case */
        else if (!curlErrno) ret = 1; /* Got smth to read */
        else ret = -1;
        return calc_post_result(reply, ret);
    }
}

int lib_http_get_file(lib_http_conn_t gf_conn, FILE* rx_file) {
    CURLcode curlResult;
    long httpResponseCode = 0;
    long httpConnectCode = 0;
    long curlErrno = 0;
    http_handler_t* handler = NULL;

    double connectDuration = 0.0;
    double nameResolvingDuration = 0.0;
    double transferDuration = 0.0;

    assert(rx_file);

    if(handler = check_conn(gf_conn), handler == NULL) return -1;

    if (curlResult = curl_easy_setopt(handler->hndlr, CURLOPT_WRITEFUNCTION, file_writer), curlResult != CURLE_OK) goto out;
    if (curlResult = curl_easy_setopt(handler->hndlr, CURLOPT_WRITEDATA, &rx_file), curlResult != CURLE_OK) goto out;

    curlResult = curl_easy_perform(handler->hndlr);

    curl_easy_getinfo(handler->hndlr, CURLINFO_APPCONNECT_TIME, &connectDuration );
    curl_easy_getinfo(handler->hndlr, CURLINFO_NAMELOOKUP_TIME, &nameResolvingDuration );
    curl_easy_getinfo(handler->hndlr, CURLINFO_TOTAL_TIME, &transferDuration );
    curl_easy_getinfo(handler->hndlr, CURLINFO_RESPONSE_CODE, &httpResponseCode );
    curl_easy_getinfo(handler->hndlr, CURLINFO_HTTP_CONNECTCODE, &httpConnectCode );

    if (httpResponseCode >= 300 || httpConnectCode >= 300) {
        pu_log(LL_ERROR, "lib_http_get_file: HTTP error response code:%ld, connect code:%ld", httpResponseCode, httpConnectCode);
        curlErrno = EHOSTUNREACH;
        goto out;
    }
    if (nameResolvingDuration >= 2.0) {
        pu_log(LL_WARNING, "lib_http_get_file: connectDuration=%.2lf, nameResolvingDuration=%.2lf, transferDuration=%.2lf", connectDuration, nameResolvingDuration, transferDuration);
    }
    else {
        pu_log(LL_DEBUG, "lib_http_get_file: connectDuration=%.2lf, nameResolvingDuration=%.2lf, transferDuration=%.2lf", connectDuration, nameResolvingDuration, transferDuration);
    }
    if (curlResult != CURLE_OK) {
        if (curlResult == CURLE_ABORTED_BY_CALLBACK) {
            curlErrno = EAGAIN;
            pu_log(LL_DEBUG, "lib_http_get_file: quitting curl transfer: %d %s", curlErrno, strerror((int)curlErrno));
        }
        else {
            if (curl_easy_getinfo(handler->hndlr, CURLINFO_OS_ERRNO, &curlErrno) != CURLE_OK) {
                curlErrno = ENOEXEC;
                pu_log(LL_ERROR, "lib_http_get_file: curl_easy_getinfo");
            }
            if (curlResult == CURLE_OPERATION_TIMEDOUT) curlErrno = ETIMEDOUT; /* time out error must be distinctive */
            else if (curlErrno == 0) curlErrno = ENOEXEC; /* can't be equalt to 0 if curlResult != CURLE_OK */

            pu_log(LL_WARNING, "lib_http_get_file: %s, %s for url %s", curl_easy_strerror(curlResult), strerror((int) curlErrno), handler->url);
        }
    }
out:
    if((curlErrno == EAGAIN) || (curlErrno == ETIMEDOUT)) return 0;   /* timrout case */
    if(!curlErrno) return 1; /* Got smth uploaded */
    return -1;
}

/******************************************
    Local functions implementation
*/

static http_handler_t** alloc_conn_pull(unsigned connections_max) {
    http_handler_t** ret = malloc(connections_max*sizeof(http_handler_t*));
    if(!ret) {
        pu_log(LL_ERROR, "create_conn_pull: Can't allocate %d bytes memory", connections_max*sizeof(http_handler_t*));
        return NULL;
    }
    unsigned int i;
    for(i = 0; i < connections_max; i++) ret[i] = NULL;
    return ret;
}

static void bzero_conn(http_handler_t* conn) {
    conn->hndlr = NULL;
    bzero(conn->url, sizeof(conn->url));
    bzero(conn->rx_buf, sizeof(conn->rx_buf));
    bzero(conn->err_buf, sizeof(conn->err_buf));
    conn->inBoundCommInfo.buffer = conn->rx_buf;
    conn->inBoundCommInfo.size = sizeof(conn->rx_buf);
    conn->slist = NULL;
}

static lib_http_conn_t get_free_conn(http_handler_t** conn_array, unsigned int conn_max) {
    unsigned int i;
    for(i = 0; i < conn_max; i++) {
        if(conn_array[i] == NULL) {		/* Got vacant place */
            conn_array[i] = malloc(sizeof(http_handler_t));
            if(!conn_array[i]) {
                pu_log(LL_ERROR, "get_free_conn: Can't allocate %d bytes memory", sizeof(http_handler_t));
                return conn_max;
            }
            else {
                bzero_conn(conn_array[i]);
                return i;
            }
        }
    }
    return conn_max;
}

static http_handler_t* check_conn(lib_http_conn_t conn) {
    assert(conn < CONN_ARRAY_SIZE); assert(CONN_ARRAY[conn]);
    return CONN_ARRAY[conn];
}

/*
    Calc http_write (POST) result; fill the reply if needed.
    Return 1 if OK, 0 if connectivity problems, -1 if error
*/
/*  TODO! Here is the place to react on message status level!

ACK             The message was received and parsed successfully. The device or gateway should send the next message
                    according to its upload interval.
CONT            The message was received and parsed successfully. The device or gateway should continue sending measured
                    data because the user is actively watching the screen and expecting real-time feedback.
ERR	            Error reading this data, retry sending it.
ERR_FORMAT      The server couldn't parse your data. Check your XML or JSON format.
UNKNOWN	        The server doesn't recognize your device, it isn't registered to anybody's account.
UNAUTHORIZED	When the device is registered with bidirectional authentication enabled (authToken=true), you must
                        include your device's original cryptographic authentication token in the HTTP header of every
                        request it sends to the server. This error indicates the authentication token you provided does
                        not match the authentication token stored on the server.
 */
static lib_http_post_result_t calc_post_result(char* result, int rc) {
    lib_http_post_result_t ret;

    if(rc < 0) {    /* Some curl-level problem - everything logged */
        ret = LIB_HTTP_POST_ERROR;
    }
    else if (!rc) {  /* rc == 0 technically write is OK */
        ret = LIB_HTTP_POST_RETRY;
    }
    else { /* ...and what we got? rc == 1 */
        cJSON* obj = cJSON_Parse(result);
        if(!obj) {
            ret = LIB_HTTP_POST_ERROR;
        }
        else {
            cJSON* item = cJSON_GetObjectItem(obj, "status");
            if(!item) {
                ret = LIB_HTTP_POST_ERROR;
            }
            else if(!strcmp(item->valuestring, "ACK") || !strcmp(item->valuestring, "CONT")) {
                ret = LIB_HTTP_POST_OK;
            }
            else if(!strcmp(item->valuestring, "ERR")) {
                pu_log(LL_WARNING, "Cloud couldn't read the request from Proxy %s", result);
                ret = LIB_HTTP_POST_RETRY;
            }
            else if(!strcmp(item->valuestring, "ERR_FORMAT")) {
                pu_log(LL_WARNING, "Cloud answered for bad request from Proxy %s", result);
                ret = LIB_HTTP_POST_OK;
            }
            else if(!strcmp(item->valuestring, "UNAUTHORIZED")) {
                pu_log(LL_WARNING, "Cloud answered for unauthorized request from Proxy %s", result);
                ret = LIB_HTTP_POST_OK;
            }
            else if(!strcmp(item->valuestring, "UNKNOWN")) {
                pu_log(LL_WARNING, "Cloud answered for received unknown request from Proxy %s", result);
                ret = LIB_HTTP_POST_OK;
            }
            else {  /* UNKNOWN, ... - let's wait untill somewhere takes a look on poor cycling modem */
                pu_log(LL_WARNING, "Cloud answered strange request from Proxy %s", result);
                ret = LIB_HTTP_POST_RETRY;
            }
            cJSON_Delete(obj);
        }
    }
    return ret;
}

/**********************************************************************************************//**
 * @brief   Called when a message has to be received from the server. this is a standard streamer
 *              if the size of the data to read, equal to size*nmemb, the function can return
 *              what was read and the function will be called again by libcurl.
 *
 * @param   ptr: where the received message resides
 * @param   size: size*nmemb == number of bytes to read
 * @param   nmemb: size*nmemb == number of bytes to read
 * @param   userp: ptr to where the message will be written -> inputted by CURLOPT_WRITEDATA call below
 *
 * @return  number of bytes that were written
 ***************************************************************************************************/
static size_t writer(void *ptr, size_t size, size_t nmemb, void *userp) {
    HttpIoInfo_t *dataToRead = (HttpIoInfo_t *) userp;
    char *data = (char *)ptr;
    if (dataToRead == NULL || dataToRead->buffer == NULL) {
        pu_log(LL_ERROR, "writer: dataToRead == NULL");
        return 0;
    }

    /* keeping one byte for the null byte */
    if((strlen(dataToRead->buffer)+(size * nmemb)) > (dataToRead->size - 1))
    {
#if __WORDSIZE == 64
        pu_log(LL_WARNING, "writer: buffer overflow would result -> strlen(writeData): %lu, (size * nmemb): %lu, max size: %u",
#else
                //mlevitin pu_log(LL_WARNING, ("writer: buffer overflow would result -> strlen(writeData): %u, (size * nmemb): %u, max size: %u",
		pu_log(LL_WARNING, "writer: buffer overflow would result -> strlen(writeData): %u, (size * nmemb): %u, max size: %u",
#endif
               strlen(dataToRead->buffer), (size * nmemb), dataToRead->size);
        return 0;
    }
    else
    {

    }
    strncat(dataToRead->buffer, data, (size * nmemb));
    return (size * nmemb);
}

static long file_writer(void *buffer, size_t size, size_t nmemb, void *stream) {
    FILE* out= *((FILE **)stream);
    if(!out) {
        return -1; /* failure, bad fd passed */
    }
    return fwrite(buffer, size, nmemb, out);
}

/**
 * @brief   Called when a message has to be sent to the server. this is a standard streamer
 *              if the size of the data to write is larger than size*nmemb, this function
 *              will be called several times by libcurl.
 *
 * @param   ptr: where data has to be written
 * @param   size: size*nmemb == maximum number of bytes that can be written each time
 * @param   nmemb: size*nmemb == maximum number of bytes that can be written each time
 * @param   userp: ptr to message to write -> inputted by CURLOPT_READDATA call below
 *
 * @return  number of bytes that were written
 **/
static size_t read_callback(void *ptr, size_t size, size_t nmemb, void *userp) {
    HttpIoInfo_t *dataToWrite = (HttpIoInfo_t *) userp;
    int dataWritten = 0;
    if (dataToWrite == NULL || dataToWrite->buffer == NULL) {
        pu_log(LL_ERROR, "read_callback: dataToWrite == NULL");
        return 0;
    }
    if (size * nmemb < 1) {
        pu_log(LL_ERROR, "size * nmemb < 1");
        return 0;
    }
    if (dataToWrite->size > 0) {
        if (dataToWrite->size > (size * nmemb)) {
            dataWritten = size * nmemb;
        }
        else {
            dataWritten = dataToWrite->size;
        }
        memcpy (ptr, dataToWrite->buffer, dataWritten);
        dataToWrite->buffer += dataWritten;
        dataToWrite->size -= dataWritten;

        return dataWritten; /* we return 1 byte at a time! */
    }
    return 0;
}
#ifdef LIBHTTP_CURL_DEBUG
/***************************************************************
        Debug part
    Copypizded from https://curl.haxx.se/libcurl/c/debug.html
*/
static
void dump(const char *text,
          FILE *stream, unsigned char *ptr, size_t size,
          char nohex)
{
  size_t i;
  size_t c;

  unsigned int width=0x10;

  if(nohex)
    /* without the hex output, we can fit more on screen */
    width = 0x40;

  fprintf(stream, "%s, %10.10ld bytes (0x%8.8lx)\n",
          text, (long)size, (long)size);

  for(i=0; i<size; i+= width) {

    fprintf(stream, "%4.4lx: ", (long)i);

    if(!nohex) {
      /* hex not disabled, show it */
      for(c = 0; c < width; c++)
        if(i+c < size)
          fprintf(stream, "%02x ", ptr[i+c]);
        else
          fputs("   ", stream);
    }

    for(c = 0; (c < width) && (i+c < size); c++) {
      /* check for 0D0A; if found, skip past and start a new line of output */
      if(nohex && (i+c+1 < size) && ptr[i+c]==0x0D && ptr[i+c+1]==0x0A) {
        i+=(c+2-width);
        break;
      }
      fprintf(stream, "%c",
              (ptr[i+c]>=0x20) && (ptr[i+c]<0x80)?ptr[i+c]:'.');
      /* check again for 0D0A, to avoid an extra \n if it's at width */
      if(nohex && (i+c+2 < size) && ptr[i+c+1]==0x0D && ptr[i+c+2]==0x0A) {
        i+=(c+3-width);
        break;
      }
    }
    fputc('\n', stream); /* newline */
  }
  fflush(stream);
}

static
int my_trace(CURL *handle, curl_infotype type,
             char *data, size_t size,
             void *userp)
{
  struct data *config = (struct data *)userp;
  const char *text;
  (void)handle; /* prevent compiler warning */

  switch(type) {
  case CURLINFO_TEXT:
    fprintf(stderr, "== Info: %s", data);
  default: /* in case a new one is introduced to shock us */
    return 0;

  case CURLINFO_HEADER_OUT:
    text = "=> Send header";
    break;
  case CURLINFO_DATA_OUT:
    text = "=> Send data";
    break;
  case CURLINFO_SSL_DATA_OUT:
    text = "=> Send SSL data";
    break;
  case CURLINFO_HEADER_IN:
    text = "<= Recv header";
    break;
  case CURLINFO_DATA_IN:
    text = "<= Recv data";
    break;
  case CURLINFO_SSL_DATA_IN:
    text = "<= Recv SSL data";
    break;
  }

  dump(text, stderr, (unsigned char *)data, size, config->trace_ascii);
  return 0;
}
#endif