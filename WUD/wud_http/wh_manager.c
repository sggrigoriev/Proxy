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
    Created by gsg on 30/01/17.
*/

#include <pthread.h>
#include <string.h>
#include <errno.h>

#include "lib_http.h"
#include "pu_logger.h"

#include "wc_settings.h"
#include "wa_reboot.h"
#include "wh_manager.h"

/*************************************************************
    Local data
*/
static pthread_mutex_t frd_mutex = PTHREAD_MUTEX_INITIALIZER;     /* Protect parallel file i/o opertations */
static pthread_mutex_t wr_mutex= PTHREAD_MUTEX_INITIALIZER;       /* To prevent double penetration to write */

static unsigned int CONNECTIONS_TOTAL = 2;    /*Regular post, + file GET) */
static lib_http_conn_t post_conn = -1;

/*************************************************************
    Local functions
    NB! Local functions are not thread-protected!
*/
/*************************************************************
 *  Open POST connection
 * @param conn  - connection descriptor
 * @return  - 1 if OK, 0 if not
 */
static int get_post_connection(lib_http_conn_t* conn);

/*************************************************************
 *  POST the message to the cloud (no thread protection)
 * @param conn          - POST connection descriptor
 * @param msg           - message to be sent
 * @param reply         - buffer to receive cloud reply
 * @param reply_size    - reply buffer size
 * @param auth_token    - gateway authentication token
 * @return  -  1 if OK and 0 if error
 */
static int _post(lib_http_conn_t conn, const char* msg, char* reply, size_t reply_size, const char* auth_token);

/*************************************************************
 * Public functions implementation
 */
void wh_mgr_init(){
    if(!lib_http_init(CONNECTIONS_TOTAL)) {
        lib_http_close();
        wa_reboot();
    }
}
void wh_mgr_destroy(){
    pthread_mutex_lock(&wr_mutex);
    pthread_mutex_lock(&frd_mutex);
    lib_http_close();
    pthread_mutex_unlock(&wr_mutex);
    pthread_mutex_unlock(&frd_mutex);
}

void wh_reconnect() {
    pthread_mutex_lock(&wr_mutex);
    pthread_mutex_lock(&frd_mutex);

    lib_http_eraseConn(&post_conn);

    if(get_post_connection(&post_conn)) {
        pthread_mutex_unlock(&wr_mutex);
        pthread_mutex_unlock(&frd_mutex);
        return;
    }
    pthread_mutex_unlock(&wr_mutex);
    pthread_mutex_unlock(&frd_mutex);
    lib_http_close();
    wa_reboot();
}

int wh_write(char* buf, char* resp, size_t resp_size) {
    char auth_token[LIB_HTTP_AUTHENTICATION_STRING_SIZE];
    pthread_mutex_lock(&wr_mutex);
        wc_getAuthToken(auth_token, sizeof(auth_token));
        int ret = _post(post_conn, buf, resp, resp_size, auth_token);
    pthread_mutex_unlock(&wr_mutex);

    return ret;
}

int wh_read_file(const char* file_with_path,  const char* url, unsigned int attempts_amount) {
    FILE* rx_fd = NULL;
    int ret = 0;
    lib_http_conn_t conn = -1;

    pthread_mutex_lock(&frd_mutex);

    while(attempts_amount--) {
        rx_fd = fopen(file_with_path, "wb");
        if(!rx_fd) {
            pu_log(LL_ERROR, "wh_read_file: can't open %s file: %d, %s", file_with_path, errno, strerror(errno));
            goto on_finish;
        }

        conn = lib_http_createConn(LIB_HTTP_FILE_GET, url, NULL, NULL, LIB_HTTP_DEFAULT_TRANSFER_TIMEOUT_SEC);
        if(conn < 0) {
            pu_log(LL_ERROR, "wh_read_file: can't create HTTP connection to receive firmware");
            goto on_finish;
        }
        switch(lib_http_get_file(conn, rx_fd)) {
            case 1:             /* Got it! */
                pu_log(LL_INFO, "wh_read_file: download of %s succeed.", file_with_path);
                ret = 1;
                goto on_finish;
            case 0:             /* timeout... try it again and again, until the attempts_amount separates us */
                sleep(LIB_HTTP_DEFAULT_CONN_REESTABLISHMENT_DELAY_SEC);
                fclose(rx_fd);
                rx_fd = NULL;
                lib_http_eraseConn(&conn);
                pu_log(LL_WARNING, "wh_read_file: timeout reading %s - attempt # %d", file_with_path, attempts_amount);
                break;
            case -1:            /* Error. Get out of here. We can't live in such a dirty world! */
                pu_log(LL_ERROR, "wh_read_file, can't dowload the %s. Maybe in the next life...", file_with_path);
                goto on_finish;
        }
    }
    pu_log(LL_ERROR, "wh_write: all attempts to get the upgrade failed.");
    on_finish:
    pthread_mutex_unlock(&frd_mutex);
    lib_http_eraseConn(&conn);
    if(rx_fd) fclose(rx_fd);
    return ret;
}
/*************************************************************
 * Local functions implementation
 */

static int get_post_connection(lib_http_conn_t* conn) {
    char contact_url[LIB_HTTP_MAX_URL_SIZE];
    char device_id[LIB_HTTP_DEVICE_ID_SIZE];
    char auth_token[LIB_HTTP_AUTHENTICATION_STRING_SIZE];

    int err = 1;

    while(err){
        wc_getDeviceID(device_id, sizeof(device_id));
        wc_getAuthToken(auth_token, sizeof(auth_token));
        wc_getURL(contact_url, sizeof(contact_url));
        if(!strlen(device_id) || !strlen(contact_url) || !strlen(auth_token)) {
            pu_log(LL_ERROR, "wh_mgr_start: WUD connection parameters are not set");
            return 0;
        }
/* Open POST connection */
        if(*conn = lib_http_createConn(LIB_HTTP_CONN_POST, contact_url, auth_token, device_id, LIB_HTTP_DEFAULT_CONNECT_TIMEOUT_SEC), *conn < 0) {
            pu_log(LL_ERROR, "get_post_connection: Can't create POST connection descriptor for %s", contact_url);
            lib_http_eraseConn(conn);
            sleep(LIB_HTTP_DEFAULT_CONN_REESTABLISHMENT_DELAY_SEC);
            continue;
        }
        err = 0;    /* Bon vouage! */
    }
    pu_log(LL_INFO, "WUD connected to cloud by URL %s", contact_url);
    pu_log(LL_DEBUG, "Device ID = %s, auth_token = %s", device_id, auth_token);

    return 1;
}

static int _post(lib_http_conn_t conn, const char* msg, char* reply, size_t reply_size, const char* auth_token) {
    int out = 0;
    int ret = 0;
    int retries = LIB_HTTP_MAX_POST_RETRIES;
    if(conn < 0) return 0;
    while(!out) {
        switch (lib_http_post(conn, msg, reply, reply_size, auth_token)) {
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
                out = 1;
                break;
        }
    }
    return ret;
}

