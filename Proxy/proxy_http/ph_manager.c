//
// Created by gsg on 13/11/16.
//

#include <memory.h>
#include <pthread.h>
#include <assert.h>

#include "lib_http.h"
#include "pu_logger.h"
#include "pc_settings.h"
#include "pf_traffic_proc.h"
#include "pf_reboot.h"

#include "ph_manager.h"


//////////////////////////////////////////////////////////////////////
//
// Local data
static pthread_mutex_t rd_mutex = PTHREAD_MUTEX_INITIALIZER;        //Mutex to stop r/w operation if connection params are changing
static pthread_mutex_t wr_mutex= PTHREAD_MUTEX_INITIALIZER;       //To prevent double penetration

static lib_http_conn_t post_conn = -1;
static lib_http_conn_t get_conn = -1;
static volatile int no_conn = 1;
//////////////////////////////////////
static const char* authToken = "authToken";
//////////////////////////////////////////////////////////////////////
//
// Local functions
//
//Return 1 if OK and 0 if error
//if full reply == 1 the http_post doesn't make any parsing of replly = provides it "as is"
//No rw_mutex inside
static int _post(lib_http_conn_t post_conn, const char* msg, char* reply, size_t reply_size, const char* auth_token);
//Return 1 id OK 0 if error
static int _get(lib_http_conn_t post_conn, char* resp, size_t size);
//1. Open POST connection to main
//2. Post to get back contact url
//3. close connection to main
static int get_contact(const char* main, const char* device_id, char* conn, size_t conn_size);
//1. Close previous connections (if any)
//2. Open POST& GET conns to cloud
static int get_connections(char* conn, char* auth, char* device_id, lib_http_conn_t* post, lib_http_conn_t* get);
//1. Open POST connection w/o auth token
//2. get auth token as an answer to POST
//3 Close POST connectopm
static int get_auth_token(const char* conn, const char* device_id, char* auth_token, size_t size);
//////////////////////////////////////////////////////////////////////
//Case of initiation connections:
//1. Get main url & deviceId from config
//2. Get contact url from main
//3. If no auth_token, get auth token
//4. Open connections
// If error - start from 1.
//5. Save contact_url and auth_token
void ph_mgr_start() {
    char main_url[LIB_HTTP_MAX_URL_SIZE];
    char contact_url[LIB_HTTP_MAX_URL_SIZE];
    char device_id[LIB_HTTP_DEVICE_ID_SIZE];
    char auth_token[LIB_HTTP_AUTHENTICATION_STRING_SIZE];

    int err = 1;
    pthread_mutex_lock(&rd_mutex);
    pthread_mutex_lock(&wr_mutex);
    no_conn = 1;
    if(!lib_http_init(2)) goto on_err;
    while (err) {
//1. Get main url & deviceId from config
        pc_getMainCloudURL(main_url, sizeof(main_url));
        pc_getProxyDeviceID(device_id, sizeof(device_id));

        if(!strlen(main_url)) {
            pu_log(LL_ERROR, "Cloud connection initiation failed. Main Cloud URL is not set.");
            goto on_err;
        }
        if(!strlen(device_id)) {
            pu_log(LL_ERROR, "Cloud connection initiation failed. Main Cloud URL is not set.");
            goto on_err;
        }
//2. Get contact url from main
        if(!get_contact(main_url, device_id, contact_url, sizeof(contact_url))) continue;  //Lets try again and again
//3. If no auth_token, get auth token
        if(pc_existsActivationToken()) {
            pc_getActivationToken(auth_token, sizeof(auth_token));
        }
        else if(!get_auth_token(contact_url, device_id, auth_token, sizeof(auth_token))) continue;
//4. Open connections
        if(!get_connections(contact_url, auth_token, device_id, &post_conn, &get_conn)) continue;
        err = 0;    //Bon vouage!
    }
    pu_log(LL_INFO, "Proxy connected to cloud by URL %s", contact_url);
    pu_log(LL_DEBUG, "Device ID = %s, auth_token = %s", device_id, auth_token);

//Save all parameters updated
    pc_saveCloudURL(contact_url);
    pc_saveActivationToken(auth_token);

    no_conn = 0;    //Allow read/write operations

    pthread_mutex_unlock(&rd_mutex);
    pthread_mutex_unlock(&wr_mutex);
    return;
on_err:
    lib_http_close();
    pthread_mutex_unlock(&rd_mutex);
    pthread_mutex_unlock(&wr_mutex);
    pf_reboot();
}
void ph_mgr_stop() {
    pthread_mutex_lock(&rd_mutex);
    pthread_mutex_lock(&wr_mutex);
    no_conn = 1;
    lib_http_close();
    pthread_mutex_unlock(&rd_mutex);
    pthread_mutex_unlock(&wr_mutex);
}
//Case when the main url came from the cloud
//0. Close permanent connections
//1. Get contact url from main
//2. Get auth token
//2. Reopen connections
//3. save new main & new contact & auth token
//if error - close evrything and make the step "initiation connections"; return 0
int ph_update_main_url(const char* new_main) {
    char contact_url[LIB_HTTP_MAX_URL_SIZE];
    char device_id[LIB_HTTP_DEVICE_ID_SIZE];
    char auth_token[LIB_HTTP_AUTHENTICATION_STRING_SIZE];

    assert(new_main);
    pthread_mutex_lock(&rd_mutex);
    pthread_mutex_lock(&wr_mutex);
    no_conn = 1;

    pc_getProxyDeviceID(device_id, sizeof(device_id));  //No check: device ID is Ok if we're here
//0. Close permanent connections
    lib_http_eraseConn(post_conn);
    lib_http_eraseConn(get_conn);
//1. Get contact url from main
    if(!get_contact(new_main, device_id, contact_url, sizeof(contact_url))) goto on_err;
//2. Get auth token
    if(!get_auth_token(contact_url, device_id, auth_token, sizeof(auth_token))) goto on_err;
//3. Open connections
    if(!get_connections(contact_url, auth_token, device_id, &post_conn, &get_conn)) goto on_err;

    pu_log(LL_INFO, "Proxy reconnected with the main url = %s; contact url = %s", new_main, contact_url);
    pu_log(LL_DEBUG, "ph_update_main_url: auth_token = %s", auth_token);
//3. save new main & new contact & auth token
    pc_saveMainCloudURL(new_main);
    pc_saveCloudURL(contact_url);
    pc_saveActivationToken(auth_token);
    no_conn = 0;    //Allow read/write operations

    pthread_mutex_unlock(&rd_mutex);
    pthread_mutex_unlock(&wr_mutex);
    return 1;
//if error - close evrything and make the step "initiation connections"; return 0
on_err:
    pthread_mutex_unlock(&rd_mutex);
    pthread_mutex_unlock(&wr_mutex);
    ph_mgr_stop();
    ph_mgr_start();
    return 0;
}
//Case of connection error
//0. Close permament connections
//1. Get contact url
//2. Open permanent connections
//3. if error start from 1
//4. save new contact url
void ph_reconnect() {
    char main_url[LIB_HTTP_MAX_URL_SIZE];
    char contact_url[LIB_HTTP_MAX_URL_SIZE];
    char device_id[LIB_HTTP_DEVICE_ID_SIZE];
    char auth_token[LIB_HTTP_AUTHENTICATION_STRING_SIZE];

    pc_getMainCloudURL(main_url, sizeof(main_url));
    pc_getProxyDeviceID(device_id, sizeof(device_id));
    pc_getActivationToken(auth_token, sizeof(auth_token));

    pthread_mutex_lock(&rd_mutex);
    pthread_mutex_lock(&wr_mutex);
    no_conn = 1;

    int err = 1;
    while(err) {
//0. Close permanent connections
        lib_http_eraseConn(post_conn);
        lib_http_eraseConn(get_conn);
//1. Get contact url
        if(!get_contact(main_url, device_id, contact_url, sizeof(contact_url))) continue;  //Lets try again and again
//2. open connections

        if(!get_connections(contact_url, auth_token, device_id, &post_conn, &get_conn)) continue;
        pu_log(LL_INFO, "Proxy reconnected with the main url = %s; contact url = %s", main_url, contact_url);

        err = 0;
        pc_saveCloudURL(contact_url);
    }
    no_conn = 0;
    pthread_mutex_unlock(&rd_mutex);
    pthread_mutex_unlock(&wr_mutex);
}
//Case of periodic update the contact url
//0. Close permament conections
//1. Get contact url from main url
//2. Reopen connections
//If error - open connections with previous contact url
//If error again - start from step 1
//3. Save new contact url
void ph_update_contact_url() {
    char main_url[LIB_HTTP_MAX_URL_SIZE];
    char contact_url[LIB_HTTP_MAX_URL_SIZE];
    char device_id[LIB_HTTP_DEVICE_ID_SIZE];
    char auth_token[LIB_HTTP_AUTHENTICATION_STRING_SIZE];

    pc_getMainCloudURL(main_url, sizeof(main_url));
    pc_getProxyDeviceID(device_id, sizeof(device_id));
    pc_getActivationToken(auth_token, sizeof(auth_token));

    pthread_mutex_lock(&rd_mutex);
    pthread_mutex_lock(&wr_mutex);
    no_conn = 1;

    int err = 1;
    while(err) {
//0. Close permament conections
        lib_http_eraseConn(post_conn);
        lib_http_eraseConn(get_conn);
//1. Get contact url from main url
        if(!get_contact(main_url, device_id, contact_url, sizeof(contact_url))) continue;  //Lets try again and again
//2. Open connections
//If error - open connections with previous contact url
        if(!get_connections(contact_url, auth_token, device_id, &post_conn, &get_conn)) {
            pc_getCloudURL(contact_url, sizeof(contact_url));
//If error again - start from step 1
            if(!get_connections(contact_url, auth_token, device_id, &post_conn, &get_conn)) continue;
            pu_log(LL_ERROR, "ph_update_contact_url: get back to the prevoius contact url %s", contact_url);
        }
        else {
//3. Save new contact url
            pu_log(LL_INFO, "Proxy reconnected with the main url = %s; contact url = %s", main_url, contact_url);
            pc_saveCloudURL(contact_url);
        }
        err = 0;
    }
    no_conn = 0;
    pthread_mutex_unlock(&rd_mutex);
    pthread_mutex_unlock(&wr_mutex);
}

// Communication functions
//
// 0 if TO, -1 if conn error, 1 if OK
int ph_read(char* in_buf, size_t size) {
    pthread_mutex_lock(&rd_mutex);
        int ret = lib_http_get(get_conn, in_buf, size);
    pthread_mutex_unlock(&rd_mutex);
    return ret;
}
//Returns 0 if error, 1 if OK
//In all cases when the return is <=0 the resp contains some diagnostics
int ph_write(char* buf, char* resp, size_t resp_size) {

    char auth_token[LIB_HTTP_AUTHENTICATION_STRING_SIZE];
    pthread_mutex_lock(&wr_mutex);
        pc_getActivationToken(auth_token, sizeof(auth_token));
        int ret = _post(post_conn, buf, resp, resp_size, auth_token);
    pthread_mutex_unlock(&wr_mutex);

    return ret;
}
//////////////////////////////////////////////////////////////////////
//
// Local functions
//
//Return 1 of OK and 0 if error
//No rw_mutex inside
static int _post(lib_http_conn_t post_conn, const char* msg, char* reply, size_t reply_size, const char* auth_token) {
    int out = 0;
    int ret = 0;
    int retries = LIB_HTTP_MAX_POST_RETRIES;
    if(post_conn < 0) return 0;
    while(!out) {
        switch (lib_http_post(post_conn, msg, reply, reply_size, auth_token)) {
            case LIB_HTTP_POST_ERROR:
                out = 1;
                break;
            case LIB_HTTP_POST_RETRY:
                pu_log(LL_WARNING, "_post: Connectivity problems, retry");
                if (retries-- == 0) {
                    out = 1;
                }
                else {
                    sleep(1);
                }
                break;
            case LIB_HTTP_POST_OK:
            case LIB_HTTP_POST_AUTH_TOKEN:
                out = 1;
                ret = 1;
                break;

            default:
                break;
        }
    }
    return ret;
}
//Return 1 id OK 0 if error
static int _get(lib_http_conn_t get_conn, char* resp, size_t size) {
    int ret;
    while(ret = lib_http_get(get_conn, resp, size), ret == 0) sleep(1);
    return ret;
}
//1. Open GET connection to main
//2. GET to receive the contact url
//3. close connection to main
static int get_contact(const char* main, const char* device_id, char* conn, size_t conn_size) {
    char resp[LIB_HTTP_MAX_MSG_SIZE];
    lib_http_conn_t get_conn;

    if(get_conn = lib_http_createConn(LIB_HTTP_CONN_INIT_MAIN, main, "", device_id, LIB_HTTP_DEFAULT_CONNECT_TIMEOUT_SEC), get_conn < 0) {
        pu_log(LL_ERROR, "get_contact: Can't create connection descriptor for %s", main);
        lib_http_eraseConn(get_conn);
        return 0;
    }
    if(!_get(get_conn, resp, sizeof(resp))) {
        pu_log(LL_ERROR, "get_contact: Can't get the connection url from %s", main);
        lib_http_eraseConn(get_conn);
        return 0;
    }
    strncpy(conn, resp, conn_size-1);

    lib_http_eraseConn(get_conn);
    return 1;
}
//0. Close old connections
//1. Open POST connection to contact
//2. Open GET conn to cloud
static int get_connections(char* conn, char* auth, char* device_id, lib_http_conn_t* post, lib_http_conn_t* get) {

    if(*post = lib_http_createConn(LIB_HTTP_CONN_POST, conn, auth, device_id, LIB_HTTP_DEFAULT_CONNECT_TIMEOUT_SEC), *post < 0) {
        pu_log(LL_ERROR, "get_connections: Can't create POST connection descriptor for %s", conn);
        lib_http_eraseConn(*post);
        return 0;
    }
    if(*get = lib_http_createConn(LIB_HTTP_CONN_GET, conn, auth, device_id, pc_getLongGetTO()), *get < 0) {
        pu_log(LL_ERROR, "get_connections: Can't create GET connection descriptor for %s", conn);
        lib_http_eraseConn(*get);
        return 0;
    }
    return 1;
}
static int get_auth_token(const char* conn, const char* device_id, char* auth_token, size_t size) {
    lib_http_conn_t post_d;

    if(post_d = lib_http_createConn(LIB_HTTP_CONN_POST, conn, "", device_id, LIB_HTTP_DEFAULT_CONNECT_TIMEOUT_SEC), post_d < 0) {
        pu_log(LL_ERROR, "get_auth_token: Can't create connection to the %s", conn);
        lib_http_eraseConn(post_d);
        return 0;
    }
//Get the reply with auth token
    char buf[LIB_HTTP_MAX_MSG_SIZE] = {0};

    pf_add_proxy_head(buf, sizeof(buf), device_id, 11037);
    if(!_post(post_d, buf, auth_token, size, "")) {
        pu_log(LL_ERROR, "get_auth_token: auth token request failed");
        lib_http_eraseConn(post_d);
        return 0;
    }

    lib_http_eraseConn(post_d);
    return 1;
}













