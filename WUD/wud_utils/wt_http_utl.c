//
// Created by gsg on 07/12/16.
//
#include <curl/curl.h>
#include <memory.h>
#include <pthread.h>
#include <unistd.h>
#include <lib_http.h>
#include <assert.h>
#include <pf_traffic_proc.h>

#include "pu_logger.h"
#include "wc_defaults.h"
#include "wt_http_utl.h"


//////////////////////////////////////////////////////////////////////
//
// Local data
static pthread_mutex_t local_mutex = PTHREAD_MUTEX_INITIALIZER;
static char conn_string[LIB_HTTP_MAX_URL_SIZE] = {0};
static char deviceID[30]= {0};
static char activationToken[LIB_HTTP_AUTHENTICATION_STRING_SIZE] = {0};

static char* get_activation_token(char* buf, size_t len);
/////////////////////////////////////////////////////////////
//Global functions
//
//////////////////////////////////////////////////////////////
// Initiate cURL
//Return 1 if success, 0 if error
int wt_http_curl_init() {        //Global cUrl init. Returns 0 if failed
    if(!lib_http_init()) {
        pu_log(LL_ERROR, "Error on cUrl initialiation. Something goes wrong. Exiting.");
        return 0;
    }
    return 1;
}
//////////////////////////////////////////////////////////////
// Destroy cURL
void wt_http_curl_stop() {
    lib_http_close();
}
/////////////////////////////////////////////////////////////////////////////
//pt_http_write -POST message to the cloud. If there is communication error the
//	sending is repeated PROXY_MAX_HTTP_RETRIES times. Else the answer msg returns
// Adds the head to the message: deviceID and sequence number
//
// buf - returned adress of null-terminated string
// len - max buffer length
// resp - the addres of the string with responce. NB! do not free this memory outside!!
// resp_len - length of response
// //Returns 0 if timeout, -1 if error, 1 if OK
lib_http_post_result_t wt_http_write(char* buf, size_t buf_size, char* resp, size_t resp_size) { //Returns 0 if timeout or error, 1 if OK
    char url[LIB_HTTP_MAX_URL_SIZE];
    char atoken[LIB_HTTP_AUTHENTICATION_STRING_SIZE];


    wt_http_get_cloud_conn_string(url, sizeof(url));
    get_activation_token(atoken, sizeof(atoken)-1);

    if(!strlen(url)) {
        pu_log(LL_WARNING, "wt_http_write: no connection info. Wait.");
        return LIB_HTTP_POST_RETRY;
    }
    return lib_http_post(buf, resp, resp_size, url, atoken);
}

void wt_set_connection_info(const char* connection_string, const char* device_id, const char* activation_token) {
    pthread_mutex_lock(&local_mutex);
        strncpy(conn_string, connection_string, sizeof(conn_string)-1);
        strncpy(deviceID, device_id, sizeof(deviceID)-1);
        strncpy(activationToken, activation_token, sizeof(activationToken)-1);
    pthread_mutex_unlock(&local_mutex);
}

int wt_http_gf_init() {
    pu_log(LL_ERROR, "I'm wt_http_gf_init(). Please implement me!");
    return 0;
}
void wt_http_gf_destroy() {
    pu_log(LL_ERROR, "I'm wt_http_gf_destroy(). Please implement me!");
}
int wt_http_get_file(const char* connection_string, const char* folder) { //Returns 0 if timeout or error, 1 if OK
    pu_log(LL_ERROR, "I'm wt_http_get_file(). Please implement me!");
    return 0;
}
char* wt_http_get_cloud_conn_string(char *buf, size_t size) {
    assert(buf); assert(size);
    pthread_mutex_lock(&local_mutex);
    char* ret = strncpy(buf, conn_string, size);
    buf[size-1] = '\0';
    pthread_mutex_unlock(&local_mutex);
    return ret;
}
char* wt_get_device_id(char *buf, size_t size) {
    assert(buf); assert(size);
    pthread_mutex_lock(&local_mutex);
    char* ret = strncpy(buf, deviceID, size);
    buf[size-1] = '\0';
    pthread_mutex_unlock(&local_mutex);
    return ret;
}
///////////////////////////////////////////////////////////////////
static char* get_activation_token(char* buf, size_t size) {
    assert(buf); assert(size);
    pthread_mutex_lock(&local_mutex);
    char* ret = strncpy(buf, activationToken, size);
    buf[size-1] = '\0';
    pthread_mutex_unlock(&local_mutex);
    return ret;
}