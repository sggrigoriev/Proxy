//
// Created by gsg on 15/12/16.
//


#include <errno.h>
#include <stdbool.h>
#include <string.h>
#include <curl/curl.h>

#include <eui64.h>
#include <pc_settings.h>
#include "pu_logger.h"
#include "lib_http.h"

// structure used to store data to be sent to the server.
typedef struct {
    char * buffer;
    int size;
} HttpIoInfo_t;
//
typedef struct http_timeout_t {
    long connectTimeout;
    long transferTimeout;
} http_timeout_t;
//
typedef struct http_param_t {
    http_timeout_t timeouts;
    bool verbose;
    char *password;
    char *key;
} http_param_t;

//

//Static data for persistent "GET"
static CURLSH* curlGETHandle = NULL;
static char rd_url[LIB_HTTP_MAX_URL_SIZE];
static char rd_rx_buf[LIB_HTTP_MAX_MSG_SIZE];
static char rd_errBuf[CURL_ERROR_SIZE];
static http_param_t rd_params;
static struct curl_slist* rd_slist = NULL;
static HttpIoInfo_t rd_inBoundCommInfo = {rd_rx_buf, sizeof(rd_rx_buf)};
//
//Static functions declaration
//
//Called when a message HAS TO BE SENT to the server. this is a standard streamer
//if the size of the data to write is larger than size*nmemb, this function will be called several times by libcurl.
static size_t read_callback(void *ptr, size_t size, size_t nmemb, void *userp);
//Called when a message HAS TO BE RECEIVED from the server. this is a standard streamer
//if the size of the data to read, equal to size*nmemb, the function can return what was read
// and the function will be called again by libcurl.
static size_t writer(void *ptr, size_t size, size_t nmemb, void *userp);
//return 1 of OK, 0 if not
int lib_http_init() {
    if (curl_global_init(CURL_GLOBAL_ALL) != 0) {
        pu_log(LL_ERROR, "Error on cUrl initialiation. Something goes wrong. Exiting.");
        return 0;
    }
    return 1;
}
void lib_http_close() {
    curl_global_cleanup();
}

//Return 1 if OK, 0 if error.All errors put into log inside
int lib_http_create_get_persistent_conn(const char *url, const char* auth_token, const char* deviceID, unsigned int conn_to) {

    CURLcode curlResult = CURLE_OK;
    char buf[LIB_HTTP_AUTHENTICATION_STRING_SIZE+50];

//Initiate static connection attributes
    strncpy(rd_url, url, sizeof(rd_url));
    bzero(rd_rx_buf, sizeof(rd_rx_buf));
    bzero(rd_rx_buf, sizeof(rd_rx_buf));
    bzero(&rd_params, sizeof(rd_params));

    rd_params.timeouts.connectTimeout = conn_to;
    rd_params.timeouts.transferTimeout = conn_to;

    strncat(rd_url, "?id=", sizeof(rd_url)-1);
    strncat(rd_url, deviceID, sizeof(rd_url)-1);

    snprintf(buf, sizeof(buf)-1, "&timeout=%lu", rd_params.timeouts.transferTimeout);
    strncat(rd_url, buf, sizeof(rd_url)-1);


    if(curlGETHandle = curl_easy_init(), !curlGETHandle) {
        pu_log(LL_ERROR, "lib_http_create_get_persistent_conn: cURL GET handler creation failed.");
        return 0;
    }

    if(curlResult = curl_easy_setopt(curlGETHandle, CURLOPT_NOSIGNAL, 1L), curlResult != CURLE_OK) goto out;
/*
    if(curlResult = curl_easy_setopt(curlGETHandle, CURLOPT_CONNECTTIMEOUT, rd_params.timeouts.connectTimeout), curlResult != CURLE_OK) goto out;
    if(curlResult = curl_easy_setopt(curlGETHandle, CURLOPT_TIMEOUT, rd_params.timeouts.transferTimeout), curlResult != CURLE_OK) goto out;
*/
    if(curlResult = curl_easy_setopt(curlGETHandle, CURLOPT_URL, rd_url), curlResult != CURLE_OK) goto out;

    if(curlResult = curl_easy_setopt(curlGETHandle, CURLOPT_ERRORBUFFER, rd_errBuf), curlResult != CURLE_OK) goto out;

    // CURLOPT_WRITEFUNCTION and CURLOPT_WRITEDATA in this context refers to
    // data received from the server... so curl will write data to us.
    if(curlResult = curl_easy_setopt(curlGETHandle, CURLOPT_WRITEFUNCTION, writer), curlResult != CURLE_OK) goto out;
    if(curlResult = curl_easy_setopt(curlGETHandle, CURLOPT_WRITEDATA, &rd_inBoundCommInfo), curlResult != CURLE_OK) goto out;
/*
    if(curlResult = curl_easy_setopt(curlGETHandle, CURLOPT_BUFFERSIZE, sizeof(rd_rx_buf)), curlResult != CURLE_OK) goto out;
*/
    rd_slist = curl_slist_append(rd_slist, "User-Agent: IOT Proxy");
    snprintf(buf, sizeof(buf), "PPCAuthorization: esp token=%s", auth_token);
    rd_slist = curl_slist_append(rd_slist, buf);
    if(curlResult = curl_easy_setopt(curlGETHandle, CURLOPT_HTTPHEADER, rd_slist), curlResult != CURLE_OK) goto out;
    if(curlResult = curl_easy_setopt(curlGETHandle, CURLOPT_TCP_KEEPALIVE, 1L), curlResult != CURLE_OK) goto out;
    if(curlResult = curl_easy_setopt(curlGETHandle, CURLOPT_TCP_KEEPIDLE, (long)rd_params.timeouts.connectTimeout+1), curlResult != CURLE_OK) goto out;
    if(curlResult = curl_easy_setopt(curlGETHandle, CURLOPT_TCP_KEEPINTVL, (long)rd_params.timeouts.connectTimeout+1), curlResult != CURLE_OK) goto out;

    out:
    if(curlResult != CURLE_OK) {
        pu_log(LL_ERROR, "make_httpr_read_handler: %s", curl_easy_strerror(curlResult));
        ilb_http_delete_get_persistent_conn();
        return 0;
    }
    return 1;
}
void ilb_http_delete_get_persistent_conn() {
    if(rd_slist) {
        curl_slist_free_all(rd_slist);
        rd_slist = NULL;
    }
    curl_easy_cleanup(curlGETHandle);
}
//Return 1 if get msg, 0 if timeout, -1 if error. Logged inside
int lib_http_get(char* msg, size_t msg_size) {
    long httpResponseCode = 0;
    long httpConnectCode = 0;
    long curlErrno = 0;

    msg[0] = '\0';  //in case we got nothing
/*
    curl_easy_setopt(curlGETHandle, CURLOPT_TCP_KEEPALIVE, 1L);
    curl_easy_setopt(curlGETHandle, CURLOPT_TCP_KEEPIDLE, 120L);
    curl_easy_setopt(curlGETHandle, CURLOPT_TCP_KEEPINTVL, 120L);
    char url[LIB_HTTP_MAX_URL_SIZE];
    char dev_id[500];
    char buf[500];

    pc_getDeviceAddress(dev_id, sizeof(dev_id)-1);
    pc_getCloudURL(url, sizeof(url));

    strncat(url, "?id=", sizeof(url)-1);
    strncat(url, dev_id, sizeof(url)-1);

    snprintf(buf, sizeof(buf)-1, "&timeout=%lu", 120L);
    strncat(url, buf, sizeof(url)-1);

    curl_easy_setopt(curlGETHandle, CURLOPT_URL, url);
*/
    CURLcode curlResult = curl_easy_perform(curlGETHandle);

    curl_easy_getinfo(curlGETHandle, CURLINFO_RESPONSE_CODE, &httpResponseCode );
    curl_easy_getinfo(curlGETHandle, CURLINFO_HTTP_CONNECTCODE, &httpConnectCode );

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
            if (curl_easy_getinfo(curlGETHandle, CURLINFO_OS_ERRNO, &curlErrno) != CURLE_OK) {
                curlErrno = ENOEXEC;
                pu_log(LL_ERROR, "lib_http_get: curl_easy_getinfo");
            }
            if (curlResult == CURLE_OPERATION_TIMEDOUT) curlErrno = ETIMEDOUT; /// time out error must be distinctive
            else if (curlErrno == 0) curlErrno = ENOEXEC; /// can't be equalt to 0 if curlResult != CURLE_OK

            pu_log(LL_WARNING, "lib_http_get: %s, %s for url %s", curl_easy_strerror(curlResult), strerror((int) curlErrno), rd_url);
        }
        goto out;
    }
    // the following is a special case - a time-out from the server is going to return a
    // string with 1 character in it ...
    if (strlen(rd_rx_buf) > 1) {
        /* put the result into the main buffer and return */
        pu_log(LL_DEBUG, "lib_http_get: received msg length %d", strlen(rd_rx_buf));
        strncpy(msg, rd_rx_buf, msg_size-1);
    }
    else {
        pu_log(LL_DEBUG, "lib_http_get: received time-out message from the server");
        curlErrno = EAGAIN;
        goto out;
    }
    out:
    rd_rx_buf[0] = '\0';    //prepare rx_buf for new inputs
    if((curlErrno == EAGAIN) || (curlErrno == ETIMEDOUT)) return 0;   //timrout case
    if(!curlErrno) return 1; //Got smth to read
    return -1;
}
//Return 1 if OK, 0 if timeout, -1 if error. if strlen(relpy)>0 look for some answer from the server. All logging inside
//Makes new connectione every time to post amd close it after the operation
int lib_http_post(const char* msg, char* reply, size_t reply_size, const char* url, const char* auth_token) {
    char tx_buf[LIB_HTTP_MAX_MSG_SIZE];
    char rx_buf[LIB_HTTP_MAX_MSG_SIZE]; //for server answers
    struct curl_slist* slist = NULL;
    char errBuf[CURL_ERROR_SIZE];

    HttpIoInfo_t inBoundCommInfo = {rx_buf, sizeof(rd_rx_buf)};
    HttpIoInfo_t outBoundCommInfo ={tx_buf, (int)(strlen(msg)+1)};
    http_param_t params;

    CURLcode curlResult = CURLE_OK;
    CURLSH* wr_handler = NULL;

    char buf[41+LIB_HTTP_AUTHENTICATION_STRING_SIZE];

    bzero(tx_buf, sizeof(tx_buf));
    bzero(rx_buf, sizeof(rx_buf));
    bzero(errBuf, sizeof(errBuf));
    bzero(&params, sizeof(params));

    strncpy(tx_buf, msg, strlen(msg)+1);

    params.timeouts.connectTimeout = LIB_HTTP_DEFAULT_CONNECT_TIMEOUT_SEC;
    params.timeouts.transferTimeout = LIB_HTTP_DEFAULT_TRANSFER_TIMEOUT_SEC;

    slist = curl_slist_append(slist, "Content-Type: application/json");
    snprintf(buf, sizeof(buf)-1, "Content-Length: %d", outBoundCommInfo.size);
    slist = curl_slist_append(slist, buf);
//
    slist = curl_slist_append(slist, "User-Agent: IOT Proxy");
    snprintf(buf, sizeof(buf)-1, "PPCAuthorization: esp token=%s", auth_token);
    slist = curl_slist_append(slist, buf);

    if(!slist) {
        pu_log(LL_ERROR, "lib_http_post: error slist creation");
        return -1;
    }

    if(wr_handler = curl_easy_init(), !wr_handler) {
        pu_log(LL_ERROR, "lib_http_post: cURL GET handler creation failed.");
        goto out;
    }

    if(curlResult = curl_easy_setopt(wr_handler, CURLOPT_CONNECTTIMEOUT, params.timeouts.connectTimeout), curlResult != CURLE_OK) goto out;
    if(curlResult = curl_easy_setopt(wr_handler, CURLOPT_TIMEOUT, params.timeouts.transferTimeout), curlResult != CURLE_OK) goto out;
    if(curlResult = curl_easy_setopt(wr_handler, CURLOPT_URL, url), curlResult != CURLE_OK) goto out;

    if(curlResult = curl_easy_setopt(wr_handler, CURLOPT_ERRORBUFFER, errBuf), curlResult != CURLE_OK) goto out;

   if(curlResult = curl_easy_setopt(wr_handler, CURLOPT_WRITEFUNCTION, writer), curlResult != CURLE_OK) goto out;
    if(curlResult = curl_easy_setopt(wr_handler, CURLOPT_WRITEDATA, &inBoundCommInfo), curlResult != CURLE_OK) goto out;

    if(curlResult = curl_easy_setopt(wr_handler, CURLOPT_READFUNCTION, read_callback), curlResult != CURLE_OK) goto out;
    if(curlResult = curl_easy_setopt(wr_handler, CURLOPT_READDATA, &outBoundCommInfo), curlResult != CURLE_OK) goto out;

    if(curlResult = curl_easy_setopt(wr_handler, CURLOPT_BUFFERSIZE, sizeof(rx_buf)), curlResult != CURLE_OK) goto out;

    if(curlResult = curl_easy_setopt(wr_handler, CURLOPT_HTTPHEADER, slist), curlResult != CURLE_OK) goto out;

    if(curlResult = curl_easy_setopt(wr_handler, CURLOPT_HTTPPOST, 0L), curlResult != CURLE_OK) goto out;

//////////////////////////////////
    curlResult = curl_easy_perform(wr_handler);
/////////////////////////////////
    long httpResponseCode = 0;
    long httpConnectCode = 0;
    long curlErrno = 0;

    curl_easy_getinfo(wr_handler, CURLINFO_RESPONSE_CODE, &httpResponseCode );
    curl_easy_getinfo(wr_handler, CURLINFO_HTTP_CONNECTCODE, &httpConnectCode );

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
            if (curl_easy_getinfo(wr_handler, CURLINFO_OS_ERRNO, &curlErrno) != CURLE_OK) {
                curlErrno = ENOEXEC;
                pu_log(LL_ERROR, "lib_http_post: curl_easy_getinfo");
            }
            if (curlResult == CURLE_OPERATION_TIMEDOUT) curlErrno = ETIMEDOUT; /// time out error must be distinctive
            else if (curlErrno == 0) curlErrno = ENOEXEC; /// can't be equalt to 0 if curlResult != CURLE_OK

            pu_log(LL_WARNING, "lib_http_post: %s, %s for url %s", curl_easy_strerror(curlResult), strerror((int) curlErrno), url);
        }
        goto out;
    }
    // the following is a special case - a time-out from the server is going to return a
    // string with 1 character in it ...
    if (strlen(rx_buf) > 0) {
        /* put the result into the main buffer and return */
        pu_log(LL_DEBUG, "lib_http_post: received msg length %d", strlen(rx_buf));
        strncpy(reply, rx_buf, reply_size-1);
    }
    else {
        pu_log(LL_DEBUG, "lib_http_post: received time-out message from the server");
        curlErrno = EAGAIN;
        goto out;
    }
    out:
    if(slist) curl_slist_free_all(slist);
    if(wr_handler) curl_easy_cleanup(wr_handler);

    if(curlErrno == EAGAIN) return 0;   //timeout case
    if(!curlErrno) return 1; //Got smth to read
    return -1;
}
////////////////////////////////////////////////////////////////
//local functions
//
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
static size_t writer(void *ptr, size_t size, size_t nmemb, void *userp)
{
    HttpIoInfo_t *dataToRead = (HttpIoInfo_t *) userp;
    char *data = (char *)ptr;
    pu_log(LL_DEBUG, "writer: dataToRead->buffer = %s", dataToRead->buffer);
    if (dataToRead == NULL || dataToRead->buffer == NULL) {
        pu_log(LL_ERROR, "dataToRead == NULL");
        return 0;
    }

    // keeping one byte for the null byte
    if((strlen(dataToRead->buffer)+(size * nmemb)) > (dataToRead->size - 1))
    {
#if __WORDSIZE == 64
        pu_log(LL_WARNING, "buffer overflow would result -> strlen(writeData): %lu, (size * nmemb): %lu, max size: %u",
#else
                pu_log(LL_WARNING, ("buffer overflow would result -> strlen(writeData): %u, (size * nmemb): %u, max size: %u",
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
    pu_log(LL_DEBUG, "writer: dataToWrite->buffer = %s", dataToWrite->buffer);
    if (dataToWrite == NULL || dataToWrite->buffer == NULL) {
        pu_log(LL_ERROR, "dataToWrite == NULL");
        return 0;
    }
    if (size * nmemb < 1) {
        pu_log(LL_ERROR, "size * nmemb < 1");
        return 0;
    }
    if (dataToWrite->size > 0) {
        if (dataToWrite->size > (size * nmemb)) {
            dataWritten = size * nmemb;
            pu_log(LL_DEBUG, "dataToWrite->size = %u is larger than size * nmemb = %u",
                   dataToWrite->size, size * nmemb);
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