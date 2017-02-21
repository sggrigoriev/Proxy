//
// Created by gsg on 30/01/17.
//

#include <pthread.h>
#include <string.h>
#include <errno.h>

#include "lib_http.h"
#include "pu_logger.h"

#include "wc_settings.h"
#include "wa_reboot.h"
#include "wh_manager.h"

//////////////////////////////////////////////////////////////////////
//
// Local data
static pthread_mutex_t frd_mutex = PTHREAD_MUTEX_INITIALIZER;     //Puase i/o oprtations
static pthread_mutex_t wr_mutex= PTHREAD_MUTEX_INITIALIZER;       //To prevent double penetration

static unsigned int CONNECTIONS_TOTAL = 2;    //Regular post, regular get & immediate post (God bess the HTTP!)
static lib_http_conn_t post_conn = -1;
//////////////////////////////////////////////////////////////////////
//
// Local functions
//NB! Local functions are not thread-protected!
static int get_post_connection(lib_http_conn_t* conn);
//Return 1 if OK and 0 if error
//if full reply == 1 the http_post doesn't make any parsing of replly = provides it "as is"
//No rw_mutex inside
static int _post(lib_http_conn_t conn, const char* msg, char* reply, size_t reply_size, const char* auth_token);

//Init cURL and create connections pul size 2: - 1 for POST and 1 for file GET;
//Make POST connection
void wh_mgr_start() {
    pthread_mutex_lock(&wr_mutex);
    pthread_mutex_lock(&frd_mutex);

    if(!lib_http_init(CONNECTIONS_TOTAL)) goto on_err;

//Open POST connection
    if(!get_post_connection(&post_conn)) goto on_err;

    pthread_mutex_unlock(&wr_mutex);
    pthread_mutex_unlock(&frd_mutex);
    return;
on_err:
    pthread_mutex_unlock(&wr_mutex);
    pthread_mutex_unlock(&frd_mutex);
    lib_http_close();
    wa_reboot();
}
//Close cURL & erase connections pool
void wh_mgr_stop() {
    pthread_mutex_lock(&wr_mutex);
    pthread_mutex_lock(&frd_mutex);
    lib_http_close();
    pthread_mutex_unlock(&wr_mutex);
    pthread_mutex_unlock(&frd_mutex);
}
//(re)create post connection; re-read conn parameters & reconnect
void wh_reconnect(const char* new_url, const char* new_auth_token) {
    pthread_mutex_lock(&wr_mutex);
    pthread_mutex_lock(&frd_mutex);

    lib_http_eraseConn(post_conn);

    wc_setURL(new_url);
    wc_setAuthToken(new_auth_token);

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


//Returns 0 if error, 1 if OK
//In all cases when the return is <=0 the resp contains some diagnostics
int wh_write(char* buf, char* resp, size_t resp_size) {
    char auth_token[LIB_HTTP_AUTHENTICATION_STRING_SIZE];

    pthread_mutex_lock(&wr_mutex);
        wc_getAuthToken(auth_token, sizeof(auth_token));
        int ret = _post(post_conn, buf, resp, resp_size, auth_token);
    pthread_mutex_unlock(&wr_mutex);

    return ret;
}
//Returns 0 if error, 1 if OK
int wh_read_file(const char* file_with_path,  const char* url, unsigned int attempts_amount) {
    FILE* rx_fd = NULL;
    int ret = 0;

    pthread_mutex_lock(&frd_mutex);

    rx_fd = fopen(file_with_path, "wb");
    if(!rx_fd) {
        pu_log(LL_ERROR, "wh_read_file: can't open %s file: %d, %s", file_with_path, errno, strerror(errno));
        goto on_finish;
    }

    lib_http_conn_t conn = lib_http_createConn(LIB_HTTP_FILE_GET, url, NULL, NULL, LIB_HTTP_DEFAULT_TRANSFER_TIMEOUT_SEC);
    if(conn < 0) {
        pu_log(LL_ERROR, "wh_read_file: can't create HTTP connection to receive firmware");
        goto on_finish;
    }
    while(attempts_amount--) {
        switch(lib_http_get_file(conn, rx_fd)) {
            case 1: // Got it!
                pu_log(LL_INFO, "wh_read_file: download of %s succeed... And nobody beleived!", file_with_path);
                goto on_finish;
            case 0: //timeout... try again and again, until the attempts_amount separates us
                sleep(1);
                fclose(rx_fd);
                rx_fd = fopen(file_with_path, "wb");
                if(!rx_fd) {
                    pu_log(LL_ERROR, "wh_read_file: can't reopen %s", file_with_path);
                    goto on_finish;
                }
                break;
            case -1:    //Error. Get out of here. We can't live in such a dirty world!
                pu_log(LL_ERROR, "wh_read_file, can't dowload the %s. Maybe in the next life...", file_with_path);
                goto on_finish;
        }
    }
    ret = 1;    //Looks like we got the file...
    pthread_mutex_unlock(&frd_mutex);
    on_finish:
    pthread_mutex_unlock(&frd_mutex);
    lib_http_close();
    if(rx_fd) fclose(rx_fd);
    return ret;
}
/////////////////////////////////////////////////////////////////////////////
//
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
//Open POST connection
        if(*conn = lib_http_createConn(LIB_HTTP_CONN_POST, contact_url, auth_token, device_id, LIB_HTTP_DEFAULT_CONNECT_TIMEOUT_SEC), *conn < 0) {
            pu_log(LL_ERROR, "get_post_connection: Can't create POST connection descriptor for %s", contact_url);
            lib_http_eraseConn(*conn);
            sleep(1);
            continue;
        }
        err = 0;    //Bon vouage!
    }
    pu_log(LL_INFO, "WUD connected to cloud by URL %s", contact_url);
    pu_log(LL_DEBUG, "Device ID = %s, auth_token = %s", device_id, auth_token);

    return 1;
}
//Return 1 of OK and 0 if error
//No rw_mutex inside
static int _post(lib_http_conn_t conn, const char* msg, char* reply, size_t reply_size, const char* auth_token) {
    int out = 0;
    int ret = 0;
    int retries = LIB_HTTP_MAX_POST_RETRIES;
    if(conn < 0) return 0;
    while(!out) {
        switch (lib_http_post(conn, msg, reply, reply_size, auth_token)) {
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
                out = 1;
                ret = 1;
                break;

            default:
                break;
        }
    }
    return ret;
}