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
#include <sys/time.h>

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

static unsigned int CONNECTIONS_TOTAL = 4;  /* Regular post, regular get, immediate post (God bless the HTTP!), notification post or main cloug get */

/* Dedicated connection handlers */
static lib_http_conn_t post_conn = -1;
static lib_http_conn_t get_conn = -1;
static lib_http_conn_t immediate_post = -1;

/***************************************************************************************
 * Synchronizarion data and functions
 */
static volatile int io = 0;   /* increments when any IO operation starts and decrements when it ends */
static volatile int block = 0;   /* increments when the reconnect starts and decrements whan it ends */
static volatile int was_reconnect = 0; /* To prevent double reconnection */

static pthread_mutex_t cond_mutex = PTHREAD_MUTEX_INITIALIZER;        /* condition section protection */
static pthread_mutex_t reconnect_mutex = PTHREAD_MUTEX_INITIALIZER;        /* reconnect reentrance protection*/
static pthread_cond_t cond;

static void start_io();     /* wait until !block; inc io; was_reconnect = 1 */
static void stop_io();      /* dec io */
static int block_io();      /* wait until !block, block = 1; wait until !io. 0 if it is the first entrance */
static void unblock_io();   /* block = 0; */

/***************************************************
 * Local fubctions definition
 */
/***************************************************
 * (re)establishes all connections to the cloud
 * NB! No thread protection nor IO synchrinization!
 */
static void connect();
/*******************************************************************
 * Open POST connection for prev contact URL
 * Send the notification to the cloud with prev main URL
 * NB-1! Uses saved main URL, auth_token and cloud URL
 * NB-2!     pf_add_proxy_head(msg, sizeof(msg), deviceID, 11011); will be called here!
 * @param old_main      - previous main URL
 * @param old_contact   - previous contacy URL
 * @param auth_token    - GW auth token for the previous cloud
 * @param device_id     - GW device ID
 * @param new_main      - new main URL
 * @return              - 0 if bad 1 if OK
 */
static int cloud_notify(const char* old_contact, const char* auth_token, const char* device_id, const char* new_main);

/****************************************************************************
    Local functions
*/
/* Local POST implementation. No w_mutex inside
 *      post_conn   - connection handler
 *      msg         - message to send
 *      reply       - buffer for cloud answer
 *      reply_size  - buffer size
 *      auth_token  - gateway authentication token
 *  Return lib_http_io_result_t (see lib_http.h)
 */
static lib_http_io_result_t _post(lib_http_conn_t post_conn, const char* msg, char* reply, size_t reply_size, const char* auth_token);

/*************************************************************
 * Converts POST result into OK/BAD
 * @param result - _post RC
 * @return       - 0 BAD, 1 - good
 */
static int io_result_2_bool(lib_http_io_result_t result);
/* Get contact URL from the cloud, using main URL
1. Open POST connection to main
2. Post to get back contact url
3. close connection to main
        main        - main URL
        device_id   - gateway device id
        conn        - buffer for contact URL received from the cloud
        conn_size   - buffer size
    Return lib_http_post_result_t
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
static void erase_connections(lib_http_conn_t* post, lib_http_conn_t* get, lib_http_conn_t* quick_post);

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
static int test_auth_token(lib_http_conn_t post_t, const char* device_id, const char* old_token);

/*************************************************************************************************************
 * Public functions - they are thread-protected: no concurrent POST, GET or immediate response!
 */
void ph_mgr_init() {
    if(!lib_http_init(CONNECTIONS_TOTAL)) {
        lib_http_close();
        pf_reboot();
    }
}

void ph_mgr_destroy() {
    pthread_mutex_lock(&reconnect_mutex);
    block_io();
        lib_http_close();
    pthread_mutex_unlock(&reconnect_mutex);
}

int ph_update_main_url(const char* new_main) {
    char old_main[LIB_HTTP_MAX_URL_SIZE];
    char old_contact[LIB_HTTP_MAX_URL_SIZE];
    char device_id[LIB_HTTP_DEVICE_ID_SIZE];
    char contact_url[LIB_HTTP_MAX_URL_SIZE];
    char auth_token[LIB_HTTP_AUTHENTICATION_STRING_SIZE];

    assert(new_main);
    pthread_mutex_lock(&reconnect_mutex);
    block_io();

    pc_getMainCloudURL(old_main, sizeof(old_main));
    pc_getCloudURL(old_contact, sizeof(old_contact));
    pc_getAuthToken(auth_token, sizeof(auth_token));
    pc_getProxyDeviceID(device_id, sizeof(device_id));

/* Get new contact URL from new main URL - here it is just for test: does cloud works. In connect function the contact will be requested again */
    if(!get_contact(new_main, device_id, contact_url, sizeof(contact_url))) {
        /* New cloud doesn't work - go back to the previous one */
        unblock_io();
        pthread_mutex_unlock(&reconnect_mutex);
        return 0;
    }
/* Save new main URL */
    pc_saveMainCloudURL(new_main);
    pu_log(LL_INFO, "Proxy changed the Main URL from %s to %s", old_main, new_main);
/* Close permanent connections */
    erase_connections(&post_conn, &get_conn, &immediate_post);
/* Notify previous cloud about change */
    cloud_notify(old_contact, auth_token, device_id, new_main);
/* Reconnection now*/
    connect();

    unblock_io();
    pthread_mutex_unlock(&reconnect_mutex);
    return 1;
}

int ph_reconnect() {
    pthread_mutex_lock(&reconnect_mutex);
    if(block_io()) {    /* Secondary reconnect call, reconnect just was done! */
        pthread_mutex_unlock(&reconnect_mutex);
        return 0;
    }

    connect();

    unblock_io();
    pthread_mutex_unlock(&reconnect_mutex);
    return 1;
}

void ph_update_contact_url() {
    char main_url[LIB_HTTP_MAX_URL_SIZE];
    char contact_url[LIB_HTTP_MAX_URL_SIZE];
    char device_id[LIB_HTTP_DEVICE_ID_SIZE];
    char auth_token[LIB_HTTP_AUTHENTICATION_STRING_SIZE];

    pc_getMainCloudURL(main_url, sizeof(main_url));
    pc_getProxyDeviceID(device_id, sizeof(device_id));
    pc_getAuthToken(auth_token, sizeof(auth_token));

    pthread_mutex_lock(&reconnect_mutex);
    block_io();

/* 0. Close permanent connections */
    erase_connections(&post_conn, &get_conn, &immediate_post);
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
    unblock_io();
    pthread_mutex_unlock(&reconnect_mutex);
}

int ph_read(char* in_buf, size_t size) {
    int ret;
    start_io();
    lib_http_io_result_t rc;
    switch(rc = lib_http_get(get_conn, in_buf, size, 0)) {
        case LIB_HTTP_IO_ERROR:
        case LIB_HTTP_IO_UNKNOWN:
        case LIB_HTTP_IO_UNAUTH:
            ret = -1;
            break;
        case LIB_HTTP_IO_RETRY:
            ret = 0;
            break;
        case LIB_HTTP_IO_OK:
            ret = 1;
            break;
        default:
            ret = -1;
            pu_log(LL_ERROR,"ph_read: Unsupported RC from lib_http_get %d", rc);
            break;
    }
    stop_io();
    return ret;
}

int ph_write(char* buf, char* resp, size_t resp_size) {
    char auth_token[LIB_HTTP_AUTHENTICATION_STRING_SIZE];
    start_io();
    pc_getAuthToken(auth_token, sizeof(auth_token));
    int ret = io_result_2_bool(_post(post_conn, buf, resp, resp_size, auth_token));
    stop_io();

    return ret;
}

int ph_respond(char* buf, char* resp, size_t resp_size) {
    char auth_token[LIB_HTTP_AUTHENTICATION_STRING_SIZE];
    start_io();
    pc_getAuthToken(auth_token, sizeof(auth_token));
    int ret = io_result_2_bool(_post(immediate_post, buf, resp, resp_size, auth_token));
    stop_io();

    return ret;
}

/*****************************************************************************************************************
    Local functions implementation
*/
static void connect() {
    char main_url[LIB_HTTP_MAX_URL_SIZE];
    char contact_url[LIB_HTTP_MAX_URL_SIZE];
    char device_id[LIB_HTTP_DEVICE_ID_SIZE];
    char auth_token[LIB_HTTP_AUTHENTICATION_STRING_SIZE];

    pc_getMainCloudURL(main_url, sizeof(main_url));
    pc_getProxyDeviceID(device_id, sizeof(device_id));
    pc_getAuthToken(auth_token, sizeof(auth_token));

    int err = 1;
    while(err) {
/* 0. Close permanent connections */
        erase_connections(&post_conn, &get_conn, &immediate_post);
/* 1. Get contact url */
        if(!get_contact(main_url, device_id, contact_url, sizeof(contact_url))) {       /* Lets try it again and again */
            sleep(LIB_HTTP_DEFAULT_CONN_REESTABLISHMENT_DELAY_SEC);
            continue;
        }
/* 2. Make test empty post: if answer OK - use existing one. If answer is not OK - ask for new token */
        if(!get_auth_token(contact_url, device_id, auth_token, sizeof(auth_token))) {
            sleep(LIB_HTTP_DEFAULT_CONN_REESTABLISHMENT_DELAY_SEC);
            continue;
        }
/* 3. open connections */
        if(!get_connections(contact_url, auth_token, device_id, &post_conn, &get_conn, &immediate_post)) {
            sleep(LIB_HTTP_DEFAULT_CONN_REESTABLISHMENT_DELAY_SEC);
            continue;
        }
        pu_log(LL_INFO, "Proxy reconnected with the main url = %s; contact url = %s", main_url, contact_url);

        err = 0;
        pc_saveCloudURL(contact_url);
        pc_saveAuthToken(auth_token);
    }

}

static int cloud_notify(const char* old_contact, const char* old_token, const char* device_id, const char* new_main) {
    char msg[LIB_HTTP_MAX_MSG_SIZE];
    lib_http_conn_t post;

 /* 0. Create the notification message */
    pr_make_main_url_change_notification4cloud(msg, sizeof(msg), new_main, device_id);

/* 1. Add header to the message bafore sending */
    pf_add_proxy_head(msg, sizeof(msg), device_id);

/* 2. Open the connection */
    if(post = lib_http_createConn(LIB_HTTP_CONN_POST, old_contact, old_token, device_id, LIB_HTTP_DEFAULT_CONNECT_TIMEOUT_SEC), post < 0) {
        pu_log(LL_ERROR, "ph_notify: Can't create POST connection descriptor for %s", old_contact);
        lib_http_eraseConn(&post);
        return 0;
    }
/* 3. Send the message */
    int ret;
    char resp[LIB_HTTP_MAX_MSG_SIZE];
    if (ret = io_result_2_bool(_post(post, msg, resp, sizeof(resp), old_token)), !ret ) {
        pu_log(LL_ERROR, "cloud_notify :Error sending. ");
    }
    else {
        pu_log(LL_INFO, "cloud_notify: Main URL change notification %s sent to cloud", msg);
    }
/* 4. Close connection */
    lib_http_eraseConn(&post);

    return ret;
}

static lib_http_io_result_t _post(lib_http_conn_t post_conn, const char* msg, char* reply, size_t reply_size, const char* auth_token) {
    reply[0] = '\0';

    int out = 0;
    lib_http_io_result_t ret = LIB_HTTP_IO_ERROR;
    int retries = LIB_HTTP_MAX_POST_RETRIES;
    if(post_conn < 0) return ret;
    while(!out) {
        ret = lib_http_post(post_conn, msg, reply, reply_size, auth_token);
        switch (ret) {
            case LIB_HTTP_IO_UNKNOWN:
            case LIB_HTTP_IO_UNAUTH:
                out = 1;
                break;
            case LIB_HTTP_IO_RETRY:
            case LIB_HTTP_IO_ERROR:
                pu_log(LL_WARNING, "_post: Connectivity problems, retry");
                if (retries-- == 0) {
                    out = 1;
                }
                else {
                    sleep(LIB_HTTP_DEFAULT_CONN_REESTABLISHMENT_DELAY_SEC);
                }
                break;
            case LIB_HTTP_IO_OK:
                out = 1;
                break;

            default:
                break;
        }
    }
    return ret;
}

static int io_result_2_bool(lib_http_io_result_t result) {
    return (result == LIB_HTTP_IO_OK);
}

static int get_contact(const char* main, const char* device_id, char* conn, size_t conn_size) {
    char resp[LIB_HTTP_MAX_MSG_SIZE];
    lib_http_conn_t get_conn;

    lib_http_conn_type_t conn_type = (pc_setSSLForCloudURLRequest())?LIB_HTTP_CONN_INIT_MAIN:LIB_HTTP_CONN_INIT_MAIN_NOSSL;


    if(get_conn = lib_http_createConn(conn_type, main, "", device_id, LIB_HTTP_DEFAULT_CONNECT_TIMEOUT_SEC), get_conn < 0) {
        pu_log(LL_ERROR, "get_contact: Can't create connection descriptor for %s", main);
        lib_http_eraseConn(&get_conn);
        return 0;
    }
    lib_http_io_result_t rc = lib_http_get(get_conn, resp, sizeof(resp), 1);
    if(rc != LIB_HTTP_IO_OK) {
        pu_log(LL_ERROR, "get_contact: Can't get the connection url from %s. RC = %d. Responce = %s", main, rc, resp);
        lib_http_eraseConn(&get_conn);
        return 0;
    }
    if(!strlen(resp)) {
        pu_log(LL_ERROR, "get_contact: No data returned by %s", main);
        lib_http_eraseConn(&get_conn);
        return 0;

    }
    strncpy(conn, resp, conn_size-1);
    lib_http_eraseConn(&get_conn);
    pu_log(LL_DEBUG, "get_contact: Contact URL is %s", conn);
    return 1;
}

static int get_connections(char* conn, char* auth, char* device_id, lib_http_conn_t* post, lib_http_conn_t* get, lib_http_conn_t* post1) {

    if(*post = lib_http_createConn(LIB_HTTP_CONN_POST, conn, auth, device_id, LIB_HTTP_DEFAULT_CONNECT_TIMEOUT_SEC), *post < 0) {
        pu_log(LL_ERROR, "get_connections: Can't create POST connection descriptor for %s", conn);
        lib_http_eraseConn(post);
        return 0;
    }
    if(*get = lib_http_createConn(LIB_HTTP_CONN_GET, conn, auth, device_id, pc_getLongGetTO()), *get < 0) {
        pu_log(LL_ERROR, "get_connections: Can't create GET connection descriptor for %s", conn);
        lib_http_eraseConn(get);
        return 0;
    }
    if(*post1 = lib_http_createConn(LIB_HTTP_CONN_POST, conn, auth, device_id, LIB_HTTP_DEFAULT_CONNECT_TIMEOUT_SEC), *post1 < 0) {
        pu_log(LL_ERROR, "get_connections: Can't create IMMEDIATE POST connection descriptor for %s", conn);
        lib_http_eraseConn(post);
        return 0;
    }
    return 1;
}

static void erase_connections(lib_http_conn_t* post, lib_http_conn_t* get, lib_http_conn_t* quick_post) {
    lib_http_eraseConn(post);
    lib_http_eraseConn(get);
    lib_http_eraseConn(quick_post);
}

static int get_auth_token(const char* conn, const char* device_id, char* auth_token, size_t size) {
    lib_http_conn_t post_d;

    if(post_d = lib_http_createConn(LIB_HTTP_CONN_POST, conn, auth_token, device_id, LIB_HTTP_DEFAULT_CONNECT_TIMEOUT_SEC), post_d < 0) {
        pu_log(LL_ERROR, "get_auth_token: Can't create connection to the %s", conn);
        return 0;
    }
    if(strlen(auth_token)) {    /* We got auth_token in config */
        if (test_auth_token(post_d, device_id, auth_token)) {    /* existing token works */
            lib_http_eraseConn(&post_d);
            return 1;
        }
        pu_log(LL_ERROR, "get_auth_token: Existing auth token doesn't suit - Re-request");
    }
 /* Request and/or re-request for new token */
    if(get_new_token(post_d, device_id, auth_token, size)) {
/* We have to send the empty post with the new token to cloud */
        if(test_auth_token(post_d, device_id, auth_token)) {
            pu_log(LL_INFO, "get_auth_token: New auth token was approved by cloud");
            lib_http_eraseConn(&post_d);
            return 1;
        }
    }
    pu_log(LL_ERROR, "get_auth_token: New auth token was not approved by cloud");
    lib_http_eraseConn(&post_d);
    return 0;
}

static int get_new_token(lib_http_conn_t post_d, const char* device_id, char* new_token, size_t size) {
    char buf[LIB_HTTP_MAX_MSG_SIZE] = {0};
    char reply[LIB_HTTP_MAX_MSG_SIZE] ={0};

    pf_add_proxy_head(buf, sizeof(buf), device_id);
    int i = 0;
again:
    switch(_post(post_d, buf, reply, sizeof(reply), "")) {
         case LIB_HTTP_IO_RETRY:
             pu_log(LL_WARNING, "get_new_token: Retry. Attampt #%d", i++);
             goto again;
         case LIB_HTTP_IO_UNAUTH: {     /* cloud could return the token - lets check it! */
            cJSON *obj = cJSON_Parse(reply);
            if(!obj) {
                pu_log(LL_ERROR, "get_new_token: Can't parse the cloud reply: %s", reply);
                return 0;
            }
            cJSON* token = cJSON_GetObjectItem(obj, "authToken");
            if(!token) {
                pu_log(LL_ERROR, "get_new_token: No auth token from the cloud: %s", reply);
                cJSON_Delete(obj);
                return 0;
            }
            strncpy(new_token, token->valuestring, size-1);
            pu_log(LL_INFO, "get_new_token: Got the new auth token from the cloud: %s", new_token);
            cJSON_Delete(obj);
            return 1;
        }
        default:
            pu_log(LL_ERROR, "get_new_token: auth token request failed: %s", reply);
            return 0;
    }
}

static int test_auth_token(lib_http_conn_t post_d, const char* device_id, const char* old_token) {
    char buf[LIB_HTTP_MAX_MSG_SIZE] = {0};
    char reply[LIB_HTTP_MAX_MSG_SIZE] ={0};

    pf_add_proxy_head(buf, sizeof(buf), device_id);
    lib_http_io_result_t rc;
    int i= 0;
    do {
        if(i) pu_log(LL_WARNING, "%s: Retry: Attempt #%d", __FUNCTION__, i);
        rc = _post(post_d, buf, reply, sizeof(reply), old_token);
        i++;
    } while ((rc != LIB_HTTP_IO_OK) && (rc != LIB_HTTP_IO_UNAUTH) && ((rc != LIB_HTTP_IO_UNKNOWN)));

    return io_result_2_bool(rc);
 }
/*******************************
 * Sync functions
 */


static void start_io() {     /* wait until !block; inc io; was_reconnect = 1 */
    struct timespec timeToWait;
    struct timeval now;

    pthread_mutex_lock(&cond_mutex);
    while(block) {                              /* Wait until IO will be unblocked */
        gettimeofday(&now, NULL);
        timeToWait.tv_sec = now.tv_sec+1;
        timeToWait.tv_nsec = 0;
        pthread_cond_timedwait(&cond, &cond_mutex, &timeToWait);
    }
    io++;
    was_reconnect = 0;
    pthread_mutex_unlock(&cond_mutex);
}
static void stop_io() {      /* dec io */
    pthread_mutex_lock(&cond_mutex);
    io--;
    pthread_cond_broadcast(&cond);
    pthread_mutex_unlock(&cond_mutex);
 }
/**************************************************
 * Block http IO in all threads
 * @return 0 if it is first call, return 1 if we got reentrance - the second block_io in a row
 */
static int block_io() {
    struct timespec timeToWait;
    struct timeval now;

    pthread_mutex_lock(&cond_mutex);
    if(was_reconnect) {     /* There were no IO operations after the last reconnect - get out of here! */
        pthread_mutex_unlock(&cond_mutex);
        pu_log(LL_DEBUG, "block_io(): secondary entrance! Return w/o action.");
        return 1;
    }
    block = 1;
    while(io) {                              /* Wait until no IO operations run */
        gettimeofday(&now, NULL);
        timeToWait.tv_sec = now.tv_sec+1;
        timeToWait.tv_nsec = 0;
        pthread_cond_timedwait(&cond, &cond_mutex, &timeToWait);
    }
    was_reconnect = 1;
    pthread_mutex_unlock(&cond_mutex);
    return 0;
}
static void unblock_io() {   /* block = 0; */
    pthread_mutex_lock(&cond_mutex);
    block = 0;
    pthread_cond_broadcast(&cond);
    pthread_mutex_unlock(&cond_mutex);
}
