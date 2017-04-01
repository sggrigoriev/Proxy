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
    Created by gsg on 13/11/16.
*/

#include <memory.h>
#include <pthread.h>
#include <assert.h>

#include "cJSON.h"
#include "pu_logger.h"
#include "lib_http.h"
#include "pr_commands.h"

#include "pc_settings.h"
#include "pf_traffic_proc.h"
#include "pf_reboot.h"

#include "ph_manager.h"


/************************************************************************************************

    Local data
*/
static pthread_mutex_t rd_mutex = PTHREAD_MUTEX_INITIALIZER;        /* Mutex to stop r/w operation if connection params are changing */
static pthread_mutex_t wr_mutex= PTHREAD_MUTEX_INITIALIZER;         /* To prevent double penetration */
static pthread_mutex_t nt_mutex = PTHREAD_MUTEX_INITIALIZER;        /* Protect old connection data */

static unsigned int CONNECTIONS_TOTAL = 4;  /* Regular post, regular get, immediate post (God bless the HTTP!), notification post */

/* Dedicated connection handlers */
static lib_http_conn_t post_conn = -1;
static lib_http_conn_t get_conn = -1;
static lib_http_conn_t immediate_post = -1;


/* Previous connection data (saving when ph_update_main_url() works */
char prev_contact_url[LIB_HTTP_MAX_URL_SIZE];
char prev_main_url[LIB_HTTP_MAX_URL_SIZE];
char prev_auth_token[LIB_HTTP_AUTHENTICATION_STRING_SIZE];
/*********************************************************************/

/* Response types of auth token validity check */
typedef enum {
    PH_TEST_AT_ERR, PH_TEST_AT_OK, PH_TEST_AT_WRONG
} test_auth_token_rc_t;

/* Type of possible cloud responses */
typedef enum{
    PH_DECODE_CLOUD_REPLY_ERR, PH_DECODE_CLOUD_REPLY_TOKEN, PH_DECODE_CLOUD_REPLY_ACK,
    PH_DECODE_CLOUD_REPLY_WRONG_TOKEN
} decode_cloud_reply_rc_t;

/****************************************************************************

    Local functions
*/
/* Local POST implementation. No w_mutex inside
 *      post_conn   - connection handler
 *      msg         - message to send
 *      reply       - buffer for cloud answer
 *      reply_size  - buffer size
 *      auth_token  - gateway authentication token
 *  Return 1 if OK and 0 if error
 */
static int _post(lib_http_conn_t post_conn, const char* msg, char* reply, size_t reply_size, const char* auth_token);

/* Local GET implementation No r_mutex inside. Calls http_get until the answer or error with 1 second delay between attempts
 *      get_conn    - connection handler
 *      resp        - buffer for message received
 *      size        - buffer size
 *  Return 1 id OK 0 if error
 */
static int _get(lib_http_conn_t get_conn, char* resp, size_t size);

/* Get contact URL from the cloud, using main URL
1. Open POST connection to main
2. Post to get back contact url
3. close connection to main
        main        - main URL
        device_id   - gateway device id
        conn        - buffer for contact URL received from the cloud
        conn_size   - buffer size
    Return 1 if OK, 0 if not
*/
static int get_contact(const char* main, const char* device_id, char* conn, size_t conn_size);

/* (Re)open all connections for common work: POST, GET, immediate reply
1. Close previous connections (if any)
2. Open POST& GET conns to cloud
        conn        - contact cloud URL
        auth        - gateway authentication token
        device_id   - gateway device id
        post        - post connection descriptor
        get         - get connection descriptor
        quick_post  - post connection for immediate answer
    Return 1 if OK, 0 if error
*/
static int get_connections(char* conn, char* auth, char* device_id, lib_http_conn_t* post, lib_http_conn_t* get, lib_http_conn_t* quick_post);

/* Eraze all dedicated connections
 *      post        - post connection descriptor
 *      get         - get connection descriptor
 *      quick_post  - immediate answer connection descriptor
*/
static void erase_connections(lib_http_conn_t post, lib_http_conn_t get, lib_http_conn_t quick_post);

/* Get auth token from the cloud if needed. The existing token could be invalid...
Case1: Old modem, old token -> OK
Case2: Old modem, empty tolen -> ERR
Case3: New modem, old token -> OK; new token
Case4: Nde modem, empty token-> OK; new token
        conn        - contact cloud URL
        device_id   - gateway device id
        auth_token  - buffer for received gateway authentication token
        size        - buffer size
    Return 1 if OK, 0 if not
*/
static int get_auth_token(const char* conn, const char* device_id, char* auth_token, size_t size);

/* Request new token from the cloud
 *      post_t      - post connection handler
 *      device_id   - gateway device id
 *      new_token   - buffer for new token received
 *      size        - buffer size
 *  Return 1 if OK, 0 if error
 */
static int get_new_token(lib_http_conn_t post_t, const char* device_id, char* new_token, size_t size);

/* Test the existing token with the cloud - does cloud accept it or not
 *      post_t      - post connection handler
 *      device_id   - gateway device id
 *      old_token   - existing token (red fron the disk)
 *  Return PH_TEST_AT_ERR if error, PH_TEST_AT_OK if token accepted, PH_TEST_AT_WRONG if token does not accepted by the cloud
 */
static test_auth_token_rc_t test_auth_token(lib_http_conn_t post_t, const char* device_id, const char* old_token);

/* Convert cloud reply to the decode_cloud_reply_rc_t
 *      reply   - cloud's answer to be parsed
 *      result  - buffer for authentication token
 *      size    - bufer size
 *  Return PH_TEST_AT_ERR if error, PH_TEST_AT_OK if result got new token, PH_TEST_AT_WRONG - token is bad
*/
static decode_cloud_reply_rc_t decode_cloud_reply(const char* reply, char* result, size_t size);

/*************************************************************************************************************
 * Public functions - they are thread-protected: no concurrent POST, GET or immediate response!
 */
void ph_mgr_start() {
    char main_url[LIB_HTTP_MAX_URL_SIZE];
    char contact_url[LIB_HTTP_MAX_URL_SIZE];
    char device_id[LIB_HTTP_DEVICE_ID_SIZE];
    char auth_token[LIB_HTTP_AUTHENTICATION_STRING_SIZE];

    int err = 1;
    pthread_mutex_lock(&rd_mutex);
    pthread_mutex_lock(&wr_mutex);

    if(!lib_http_init(CONNECTIONS_TOTAL)) goto on_err;
    while (err) {
/*1. Get main url & deviceId from config */
        pc_getMainCloudURL(main_url, sizeof(main_url));
        pc_getProxyDeviceID(device_id, sizeof(device_id));
        pc_getAuthToken(auth_token, sizeof(auth_token));  /*Coud be just "" if undefined...*/

        if(!strlen(main_url)) {
            pu_log(LL_ERROR, "Cloud connection initiation failed. Main Cloud URL is not set.");
            goto on_err;
        }
        if(!strlen(device_id)) {
            pu_log(LL_ERROR, "Cloud connection initiation failed. Main Cloud URL is not set.");
            goto on_err;
        }
/* 2. Get contact url from main */
        if(!get_contact(main_url, device_id, contact_url, sizeof(contact_url))) {   /* Lets try again and again */
            sleep(LIB_HTTP_DEFAULT_CONN_REESTABLISHMENT_DELAY_SEC);
            continue;
        }
/* 3. Make test empty post: if answer OK - use existing one. If answer is not OK - ask for new token */
        if(!get_auth_token(contact_url, device_id, auth_token, sizeof(auth_token))) {
            sleep(LIB_HTTP_DEFAULT_CONN_REESTABLISHMENT_DELAY_SEC);
            continue;
        }
/* 4. Open connections */
        if(!get_connections(contact_url, auth_token, device_id, &post_conn, &get_conn, &immediate_post)) {
            sleep(LIB_HTTP_DEFAULT_CONN_REESTABLISHMENT_DELAY_SEC);
            continue;
        }
        err = 0;    /* Bon vouage! */
    }
    pu_log(LL_INFO, "Proxy connected to cloud by URL %s", contact_url);
    pu_log(LL_DEBUG, "Device ID = %s, auth_token = %s", device_id, auth_token);

/* Save all parameters updated */
    pc_saveCloudURL(contact_url);
    pc_saveAuthToken(auth_token);

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
        lib_http_close();
    pthread_mutex_unlock(&rd_mutex);
    pthread_mutex_unlock(&wr_mutex);
}

int ph_update_main_url(const char* new_main) {
    char contact_url[LIB_HTTP_MAX_URL_SIZE];
    char old_main[LIB_HTTP_MAX_URL_SIZE];
    char device_id[LIB_HTTP_DEVICE_ID_SIZE];
    char auth_token[LIB_HTTP_AUTHENTICATION_STRING_SIZE];

    assert(new_main);
    pthread_mutex_lock(&rd_mutex);
    pthread_mutex_lock(&wr_mutex);

    pc_getMainCloudURL(old_main, sizeof(old_main));
    pc_getProxyDeviceID(device_id, sizeof(device_id));  /* No check: device ID is Ok if we're here */
    pc_getAuthToken(auth_token, sizeof(auth_token));

/* Save previous values for the notifiation use */
    pthread_mutex_lock(&nt_mutex);
    pc_getCloudURL(prev_contact_url, sizeof(prev_contact_url));
    strncpy(prev_auth_token, auth_token, sizeof(prev_auth_token));
    strncpy(prev_main_url, old_main, sizeof(prev_main_url));
    pthread_mutex_unlock(&nt_mutex);
/* 0. Close permanent connections */
    erase_connections(post_conn, get_conn, immediate_post);
/* 1. Get contact url from main */
    if(!get_contact(new_main, device_id, contact_url, sizeof(contact_url))) goto on_err;
/* 2. Get auth token. If the existing one is valid it remains the same. */
    if (!get_auth_token(contact_url, device_id, auth_token, sizeof(auth_token))) goto on_err;
/* 3. Open connections */
    if(!get_connections(contact_url, auth_token, device_id, &post_conn, &get_conn, &immediate_post)) goto on_err;

    pu_log(LL_INFO, "Proxy reconnected with the main url = %s; contact url = %s", new_main, contact_url);
    pu_log(LL_DEBUG, "ph_update_main_url: auth_token = %s", auth_token);
/* 4. save new main & new contact & auth token */
    pc_saveMainCloudURL(new_main);
    pc_saveCloudURL(contact_url);
    pc_saveAuthToken(auth_token);

    pthread_mutex_unlock(&rd_mutex);
    pthread_mutex_unlock(&wr_mutex);
    return 1;
/* if error - close everything and make the step "initiation connections"; return 0 */
on_err:
    pthread_mutex_unlock(&rd_mutex);
    pthread_mutex_unlock(&wr_mutex);
    ph_mgr_stop();
    ph_mgr_start();
    return 0;
}

void ph_reconnect() {
    char main_url[LIB_HTTP_MAX_URL_SIZE];
    char contact_url[LIB_HTTP_MAX_URL_SIZE];
    char device_id[LIB_HTTP_DEVICE_ID_SIZE];
    char auth_token[LIB_HTTP_AUTHENTICATION_STRING_SIZE];

    pc_getMainCloudURL(main_url, sizeof(main_url));
    pc_getProxyDeviceID(device_id, sizeof(device_id));
    pc_getAuthToken(auth_token, sizeof(auth_token));

    pthread_mutex_lock(&rd_mutex);
    pthread_mutex_lock(&wr_mutex);

    int err = 1;
    while(err) {
/* 0. Close permanent connections */
        erase_connections(post_conn, get_conn, immediate_post);
/* 1. Get contact url */
        if(!get_contact(main_url, device_id, contact_url, sizeof(contact_url))) {       /* Lets try it again and again */
            sleep(LIB_HTTP_DEFAULT_CONN_REESTABLISHMENT_DELAY_SEC);
            continue;
        }
/* 2. open connections */

        if(!get_connections(contact_url, auth_token, device_id, &post_conn, &get_conn, &immediate_post)) {
            sleep(LIB_HTTP_DEFAULT_CONN_REESTABLISHMENT_DELAY_SEC);
            continue;
        }
        pu_log(LL_INFO, "Proxy reconnected with the main url = %s; contact url = %s", main_url, contact_url);

        err = 0;
        pc_saveCloudURL(contact_url);
    }
    pthread_mutex_unlock(&rd_mutex);
    pthread_mutex_unlock(&wr_mutex);
}

void ph_update_contact_url() {
    char main_url[LIB_HTTP_MAX_URL_SIZE];
    char contact_url[LIB_HTTP_MAX_URL_SIZE];
    char device_id[LIB_HTTP_DEVICE_ID_SIZE];
    char auth_token[LIB_HTTP_AUTHENTICATION_STRING_SIZE];

    pc_getMainCloudURL(main_url, sizeof(main_url));
    pc_getProxyDeviceID(device_id, sizeof(device_id));
    pc_getAuthToken(auth_token, sizeof(auth_token));

    pthread_mutex_lock(&rd_mutex);
    pthread_mutex_lock(&wr_mutex);

/* 0. Close permament conections */
    erase_connections(post_conn, get_conn, immediate_post);
    int err = 1;
    while(err) {
/* 1. Get contact url from main url */
        if(!get_contact(main_url, device_id, contact_url, sizeof(contact_url))) {   /* Lets try again and again */
            sleep(LIB_HTTP_DEFAULT_CONN_REESTABLISHMENT_DELAY_SEC);
            continue;
        }
/* 2. Open connections */
/* If error - open connections with previous contact url */
        if(!get_connections(contact_url, auth_token, device_id, &post_conn, &get_conn, &immediate_post)) {
            pc_getCloudURL(contact_url, sizeof(contact_url));
/* If error again - start from step 1 */
            if(!get_connections(contact_url, auth_token, device_id, &post_conn, &get_conn, &immediate_post)) {
                sleep(LIB_HTTP_DEFAULT_CONN_REESTABLISHMENT_DELAY_SEC);
                continue;
            }
            pu_log(LL_ERROR, "ph_update_contact_url: get back to the prevoius contact url %s", contact_url);
        }
        else {
/* 3. Save new contact url */
            pu_log(LL_INFO, "Proxy reconnected with the main url = %s; contact url = %s", main_url, contact_url);
            pc_saveCloudURL(contact_url);
        }
        err = 0;
    }
    pthread_mutex_unlock(&rd_mutex);
    pthread_mutex_unlock(&wr_mutex);
}

int ph_read(char* in_buf, size_t size) {
    pthread_mutex_lock(&rd_mutex);
        int ret = lib_http_get(get_conn, in_buf, size);
    pthread_mutex_unlock(&rd_mutex);
    return ret;
}

int ph_write(char* buf, char* resp, size_t resp_size) {
    char auth_token[LIB_HTTP_AUTHENTICATION_STRING_SIZE];
    pthread_mutex_lock(&wr_mutex);
        pc_getAuthToken(auth_token, sizeof(auth_token));
        int ret = _post(post_conn, buf, resp, resp_size, auth_token);
    pthread_mutex_unlock(&wr_mutex);

    return ret;
}

int ph_respond(char* buf, char* resp, size_t resp_size) {
    char auth_token[LIB_HTTP_AUTHENTICATION_STRING_SIZE];
    pthread_mutex_lock(&wr_mutex);
    pc_getAuthToken(auth_token, sizeof(auth_token));
    int ret = _post(post_conn, buf, resp, resp_size, auth_token);
    pthread_mutex_unlock(&wr_mutex);

    return ret;
}

int ph_notify(char* resp, size_t resp_size) {
    char msg[LIB_HTTP_MAX_MSG_SIZE];
    char device_id[LIB_HTTP_DEVICE_ID_SIZE];
    lib_http_conn_t post;

    pthread_mutex_lock(&nt_mutex);

    pc_getProxyDeviceID(device_id, sizeof(device_id));

/* 0. Create the notification message */
    pr_make_main_url_change_notification4cloud(msg, sizeof(msg), prev_main_url, device_id);

/* 1. Add header to the message bafore sending */
    pf_add_proxy_head(msg, sizeof(msg), device_id, 11011);

/* 2. Open the connection */
    if(post = lib_http_createConn(LIB_HTTP_CONN_POST, prev_contact_url, prev_auth_token, device_id, LIB_HTTP_DEFAULT_CONNECT_TIMEOUT_SEC), post < 0) {
        pu_log(LL_ERROR, "ph_notify: Can't create POST connection descriptor for %s", prev_contact_url);
        lib_http_eraseConn(post);
        return 0;
    }
/* 3. Send the message */
    int ret = _post(post, msg, resp, resp_size, prev_auth_token);
    if(ret) {
        pu_log(LL_DEBUG, "ph_notify: change main URL notification %s sent to  cloud", msg);
    }
/* 4. Close connection */
    lib_http_eraseConn(post);

    pthread_mutex_unlock(&nt_mutex);
    return ret;
}
/*****************************************************************************************************************

    Local functions implementation
*/
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
                    sleep(LIB_HTTP_DEFAULT_CONN_REESTABLISHMENT_DELAY_SEC);
                }
                break;
            case LIB_HTTP_POST_OK:
                out = 1;
                ret = 1;
                break;

            default:
                break;
        }
    }
    return ret;
}

/*Return 1 id OK 0 if error */
static int _get(lib_http_conn_t get_conn, char* resp, size_t size) {
    int ret;
    while(ret = lib_http_get(get_conn, resp, size), ret == 0) sleep(LIB_HTTP_DEFAULT_CONN_REESTABLISHMENT_DELAY_SEC);
    return ret;
}

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
    if(!strlen(resp)) {
        pu_log(LL_ERROR, "get_contact: No data returned by %s", main);
        lib_http_eraseConn(get_conn);
        return 0;

    }
    strncpy(conn, resp, conn_size-1);

    lib_http_eraseConn(get_conn);
    return 1;
}

static int get_connections(char* conn, char* auth, char* device_id, lib_http_conn_t* post, lib_http_conn_t* get, lib_http_conn_t* post1) {

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
    if(*post1 = lib_http_createConn(LIB_HTTP_CONN_POST, conn, auth, device_id, LIB_HTTP_DEFAULT_CONNECT_TIMEOUT_SEC), *post1 < 0) {
        pu_log(LL_ERROR, "get_connections: Can't create IMMEDIATE POST connection descriptor for %s", conn);
        lib_http_eraseConn(*post);
        return 0;
    }
    return 1;
}

static void erase_connections(lib_http_conn_t post, lib_http_conn_t get, lib_http_conn_t quick_post) {
    lib_http_eraseConn(post);
    lib_http_eraseConn(get);
    lib_http_eraseConn(quick_post);
}

static int get_auth_token(const char* conn, const char* device_id, char* auth_token, size_t size) {
    lib_http_conn_t post_d;

    if(post_d = lib_http_createConn(LIB_HTTP_CONN_POST, conn, auth_token, device_id, LIB_HTTP_DEFAULT_CONNECT_TIMEOUT_SEC), post_d < 0) {
        pu_log(LL_ERROR, "get_auth_token: Can't create connection to the %s", conn);
        goto on_err;
    }
    if(strlen(auth_token)) {    /* We got auth_token in config */
        switch(test_auth_token(post_d, device_id, auth_token)) {
            case PH_TEST_AT_OK: /* existing token works */
                goto on_ok;
            case PH_TEST_AT_WRONG:
                pu_log(LL_ERROR, "get_auth_token: Exisiting auth token doesn't suit - Re-request");
                break;
            case PH_TEST_AT_ERR:
            default:
                goto on_err;
        }
    }
/* Request and/or re-request for new token */
    if(!get_new_token(post_d, device_id, auth_token, size)) goto on_err;
/* We have to send the empty post with the new token to cloud */
    if(test_auth_token(post_d, device_id, auth_token) != PH_TEST_AT_OK) {
        pu_log(LL_ERROR, "get_auth_token: New auth token was not approved by cloud");
        goto on_err;
    }
on_ok:
    lib_http_eraseConn(post_d);
    return 1;
on_err:
    lib_http_eraseConn(post_d);
    return 0;
}

static int get_new_token(lib_http_conn_t post_d, const char* device_id, char* new_token, size_t size) {
    char buf[LIB_HTTP_MAX_MSG_SIZE] = {0};
    char reply[LIB_HTTP_MAX_MSG_SIZE] ={0};

    pf_add_proxy_head(buf, sizeof(buf), device_id, 11037);
    if(!_post(post_d, buf, reply, sizeof(reply), "")) {
        pu_log(LL_ERROR, "get_new_token: auth token request failed");
        return 0;
    }
    switch(decode_cloud_reply(reply, buf, sizeof(buf))) {
        case PH_DECODE_CLOUD_REPLY_ERR:
            return 0;
        case PH_DECODE_CLOUD_REPLY_TOKEN:
            strncpy(new_token, buf, size-1);
            return 1;
        case PH_DECODE_CLOUD_REPLY_WRONG_TOKEN:
            pu_log(LL_ERROR, "get_new_token: no token from cloud: %s", buf);
            return 0;
        case PH_DECODE_CLOUD_REPLY_ACK :
        default:
            pu_log(LL_ERROR, "get_new_token: unexpected answer from cloud %s", buf);
            return 0;
    }
    return 0;   /* just on case... */
}

static test_auth_token_rc_t test_auth_token(lib_http_conn_t post_d, const char* device_id, const char* old_token) {
    char buf[LIB_HTTP_MAX_MSG_SIZE] = {0};
    char reply[LIB_HTTP_MAX_MSG_SIZE] ={0};

    pf_add_proxy_head(buf, sizeof(buf), device_id, 11037);
    if(!_post(post_d, buf, reply, sizeof(reply), old_token)) {
        pu_log(LL_ERROR, "test_auth_token: auth token request failed");
        return PH_TEST_AT_ERR;
    }
    switch(decode_cloud_reply(reply, buf, sizeof(buf))) {
         case PH_DECODE_CLOUD_REPLY_ACK :
             return PH_TEST_AT_OK;
        case PH_DECODE_CLOUD_REPLY_WRONG_TOKEN:
            return PH_TEST_AT_WRONG;
        case PH_DECODE_CLOUD_REPLY_ERR:
        case PH_DECODE_CLOUD_REPLY_TOKEN:
        default:
            pu_log(LL_ERROR, "test_auth_token: unexpected answer from cloud %s", buf);
            return PH_TEST_AT_ERR;
    }
    return PH_TEST_AT_ERR; /* ust on case... */
}

static decode_cloud_reply_rc_t decode_cloud_reply(const char* reply, char* result, size_t size) {
    cJSON *obj = NULL;
    decode_cloud_reply_rc_t ret;

    result[0] = '\0';

    if(obj = cJSON_Parse(reply), !obj) {
        pu_log(LL_ERROR, "decode_cloud_reply: Can't parse the cloud reply: %s", reply);
        ret = PH_DECODE_CLOUD_REPLY_ERR;
        goto on_exit;
    }
    cJSON* item = cJSON_GetObjectItem(obj, "status");
    if(!item) {
        pu_log(LL_ERROR, "decode_cloud_reply: Can't find the item \"status\" in the cloud reply: %s", reply);
        ret = PH_DECODE_CLOUD_REPLY_ERR;
        goto on_exit;
    }
    if(!strcmp(item->valuestring, "ACK")) { /* Token is accepted - nothing to do */
        ret = PH_DECODE_CLOUD_REPLY_ACK;
        goto on_exit;
    }
    if(strcmp(item->valuestring, "UNAUTHORIZED")) {
        pu_log(LL_ERROR, "decode_cloud_reply: Wrong value of \"status\" field in the cloud reply. \"UNAUTHORIZED\" expected instead of %s", reply);
        ret = PH_DECODE_CLOUD_REPLY_ERR;
        goto on_exit;
    }
    cJSON* token = cJSON_GetObjectItem(obj, "authToken");
    cJSON* err = cJSON_GetObjectItem(obj, "errorMessage");
    if((!token) && (err) && (!strcmp(err->valuestring, "Wrong authentication token"))) {
        pu_log(LL_WARNING, "decode_cloud_reply: Cloud replied by \"%s\"", err->valuestring);
        ret = PH_DECODE_CLOUD_REPLY_WRONG_TOKEN;
        goto on_exit;
    }
    if(!token) {
        pu_log(LL_ERROR, "decode_cloud_reply: \"authToken\" field is not found in the cloud reply.%s", reply);
        ret = PH_DECODE_CLOUD_REPLY_ERR;
        goto on_exit;
    }
    if((token->type != cJSON_String)) {
        pu_log(LL_ERROR, "decode_cloud_reply: Wrong authToken type pased in the cloud reply.%s", reply);
        ret = PH_DECODE_CLOUD_REPLY_ERR;
        goto on_exit;
    }
    strncpy(result, token->valuestring, size-1);
    ret = PH_DECODE_CLOUD_REPLY_TOKEN;
on_exit:
    if(obj) cJSON_Delete(obj);
    return ret;
}












