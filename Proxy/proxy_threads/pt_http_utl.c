//
// Created by gsg on 13/11/16.
//

#include <memory.h>
#include <errno.h>
#include <unistd.h>
#include <assert.h>

#include "cJSON.h"
#include "pu_logger.h"
#include "pc_defaults.h"
#include "pt_http_utl.h"
#include "pc_settings.h"

#define PT_POST_TO  0
#define PT_GET_TO   1
//////////////////////////////////////////////////////////////////////
//
// Local data
static pr_write_result_t calc_post_result(char* result, size_t size, int rc);
//////////////////////////////////////////////////
int pt_http_curl_start() {
    return lib_http_init();
}
void pt_http_curl_stop() {
    lib_http_close();
}
int pt_http_read_init() {
    char url[LIB_HTTP_MAX_URL_SIZE];
    char auth_token[LIB_HTTP_AUTHENTICATION_STRING_SIZE];
    char device_id[PROXY_DEVICE_ID_SIZE];

    pc_getCloudURL(url, sizeof(url));
    pc_getActivationToken(auth_token, sizeof(auth_token));
    pc_getDeviceAddress(device_id, sizeof(device_id));

    return lib_http_create_get_persistent_conn(url, auth_token, device_id, pc_getLongGetTO());
}
void pt_http_read_destroy() {
    ilb_http_delete_get_persistent_conn();
}
// Returns 0 if timeout, actual buf len (LONG GET) if OK, < 0 if error- no need to continue retries (-errno)
int pt_http_read(char* in_buf, size_t max_len) {
    memset(in_buf, 0, max_len);

    int ret = lib_http_get(in_buf, max_len);

    pu_log(LL_DEBUG, "pt_http_read: RC = %d", ret);
    return ret;
}
/////////////////////////////////////////////////////////////////////////////
//pt_http_write -POST message to the cloud. If there is communication error the 
//	sending is repeated PROXY_MAX_HTTP_RETRIES times. Else the answer msg returns
//
// buf - returned adress of null-terminated string
// len - max buffer length
// resp - the addres of the string with responce. NB! do not free this memory outside!!
// resp_len - length of response
// Returns 0 if timeout, -1 or error, 1 if OK
pr_write_result_t pt_http_write(char* buf, char* resp, size_t resp_size) { //Returns 0 if timeout or error, 1 if OK
    char url[LIB_HTTP_MAX_URL_SIZE];
    char atoken[LIB_HTTP_AUTHENTICATION_STRING_SIZE];

    pc_getCloudURL(url, sizeof(url)-1);
    pc_getActivationToken(atoken, sizeof(atoken)-1);

    int rc = lib_http_post(buf, resp, resp_size, url, atoken);

    return calc_post_result(resp, resp_size, rc);
}
/////////////////////////////////////////////////////////////
//Calc http_write (POST) result; fill the reply if needed.
//Return 1 if OK, 0 if connectivity problems, -1 if effof
static pr_write_result_t calc_post_result(char* result, size_t size, int rc) {
    pr_write_result_t ret;

    if(rc < 0) {    //Some curl-level problem - everything logged
        result[0] = '\0';
        ret = PT_POST_ERROR;
    }
    else if (!rc) {  // rc == 0 technically write is OK
        ret = PT_POST_RETRY;
    }
    else { // ...and what we got? rc == 1
        cJSON* obj = cJSON_Parse(result);
        if(!obj) {
            ret = PT_POST_ERROR;
        }
        else {
            cJSON* item = cJSON_GetObjectItem(obj, "status");
            if(!item) {
                ret = PT_POST_ERROR;
            }
            else if(!strcmp(item->valuestring, "ACK") || !strcmp(item->valuestring, "CONT")) {
                ret = PT_POST_OK;
            }
            else if(!strcmp(item->valuestring, "ERR")) {
                ret = PT_POST_RETRY;
            }
            else if(!strcmp(item->valuestring, "ERR_FORMAT")) {
                ret = PT_POST_ERROR;
            }
            else {  //UNKNOWN, UNAUTHORIZED - let's wait untill somewhere takes a look on poor cycling modem
                ret = PT_POST_RETRY;
            }
            cJSON_Delete(obj);
        }
    }
    return ret;
}
