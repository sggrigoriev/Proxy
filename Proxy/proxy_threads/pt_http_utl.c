//
// Created by gsg on 13/11/16.
//

#include <curl/curl.h>
#include <zconf.h>
#include <memory.h>
#include <errno.h>

#include "cJSON.h"
#include "libhttpcomm.h"
#include "pu_logger.h"
#include "pc_defaults.h"
#include "pt_http_utl.h"
#include "pc_settings.h"

//////////////////////////////////////////////////////////////////////
//
// Local data
//////////////////////////////////////////////////////////////////////
//Used by pt_http_read to get info from the server
static CURLSH *curlReadHandle = NULL;
//Buffer for messages from the server
static char msgFromServer[PROXY_MAX_MSG_LEN];

//Used by pt_http_wread to post messages to the server
static CURLSH *curlWriteHandle = NULL;
/////////////////////////////////////////////////////////////
//Extract command id from server message.
//Returm commandID or 0 if no cmd_id
//{"version":"2","status":"ACK","commands":[{"commandId":4,"type":0,"deviceId":"aioxGW-GSGTest_deviceid","paramsMap":{"accessCameraSettings":"0"}}]}
static unsigned long get_command_id(const char* json_string);
/////////////////////////////////////////////////////////////
//Prepare json string:     const char* to_server = "\"responses\": [{commandId\": <commandID>, \"result\": 0}]";
//Copy the string made to the buf
//The functions needs to make immediate answer to thr server due to synchronous server's habits.
// The answer with some significant info will be sent by Agent sometime trough the pt_http_write function
static void make_json_answer(char* buf, unsigned int buf_len, unsigned long command_id);
/////////////////////////////////////////////////////////////
// Send immediate responce to the server "Got your command, relax"
// If something went wrong - repeat the answer several times, log error and return
static void send_responce_to_command(unsigned long command_id);

/////////////////////////////////////////////////////////////
//Global functions
//
//////////////////////////////////////////////////////////////
// Initiate cURL
//Return 1 if success, 0 if error
int pt_http_curl_init() {        //Global cUrl init. Returns 0 if failed
    if (curl_global_init(CURL_GLOBAL_ALL) != 0) {
        pu_log(LL_ERROR, "Error on cUrl initialiation. Something goes wrong. Exiting.");
        pu_stop_logger();
        return 0;
    }
    return 1;
}
//////////////////////////////////////////////////////////////
// Destroy cURL
void pt_http_curl_stop() {
    curl_global_cleanup();
}
//////////////////////////////////////////////////////////////
// Initiates cURL read handler
//Retuen 1 of OK, 0 if not
int pt_http_read_init() {
//Initiation part
    // Sleep briefly to obtain init messages from application
    sleep(5);

    // Initialize the shared curl library
    libhttpcomm_curlShareInit(curlReadHandle);

    return 1;
}
//////////////////////////////////////////////////////////////
// Initiates cURL write handler
//Retuen 1 of OK, 0 if not
int pt_http_write_init() {    // Returns 0 if something wrong
    //Initiation part
    // Sleep briefly to obtain init messages from application
    sleep(5);

    // Initialize the shared curl library
    libhttpcomm_curlShareInit(curlWriteHandle);

    return 1;
}
//////////////////////////////////////////////////////////////
// Destroys cURL read handler
void pt_http_read_destroy() {
    libhttpcomm_curlShareClose(curlReadHandle);
}
//////////////////////////////////////////////////////////////
// Destroys cURL write handler
void pt_http_write_destroy() {
    libhttpcomm_curlShareClose(curlWriteHandle);
}
/////////////////////////////////////////////////////////////////////////////
//pt_http_read -"Long GET: wait for messages from the server. There are several variants if behaviour:
//  a) Command received.
//      ch_read answers immideately by "responses": [{commandId": <commandID>, "result": 0}] - means I go ot
//  b) ERROR received
//      - HTTP ERR - save it to the LOG
//      - Logical error - just forward it to the agent - no special actions
//
// buf - returned adress of null-terminated string
// return 0 if timeout or actual buf len (LONG GET), -1 if error - errno with negative sign
int pt_http_read(char** buf) { // Returns 0 if timeout or actual buf len (LONG GET), -1 if error
    char url[PATH_MAX];
    char activation_token[PROXY_MAX_ACTIVATION_TOKEN_SIZE];
    http_param_t params;

    bzero(msgFromServer, sizeof(msgFromServer));
    *buf = NULL;

    memset(&params, 0 , sizeof(params));

    params.timeouts.connectTimeout = DEFAULT_HTTP_CONNECT_TIMEOUT_SEC;
    params.timeouts.transferTimeout = pc_getLongGetTO();;
    params.verbose = false;

    pc_getCloudURL(url, sizeof(url));
    snprintf(url+strlen(url), sizeof(url)-strlen(url), "?id=");
    pc_getDeviceAddress(url+strlen(url), sizeof(url)-strlen(url));
    snprintf(url+strlen(url), sizeof(url)-strlen(url), "&timeout=%lu", params.timeouts.transferTimeout);

    pu_log(LL_DEBUG, "GET URL: %s", url);

    int ret;

    pc_getActivationToken(activation_token, sizeof(activation_token));

    ret = libhttpcomm_sendMsg(curlReadHandle, CURLOPT_HTTPGET, url,
                        pc_getSertificatePath(), activation_token, NULL, 0, msgFromServer, sizeof(msgFromServer),
                        params, NULL);

    if(ret == 0) {
        if(strstr(msgFromServer, "command") != NULL) { //we have to send the responce...
            unsigned long command_id = get_command_id(msgFromServer);
            if(command_id) send_responce_to_command(command_id);
        }
            ret = strlen(msgFromServer)+1;
        *buf = msgFromServer;
    }
    else if(ret == ETIMEDOUT) ret = 0;
    else ret = -ret;    // Some error...
    pu_log(LL_DEBUG, "ch_read: RC = %d", ret);
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
// return 0 if error and 1 if OK
int pt_http_write(char* buf, unsigned int len, char** resp, unsigned int* resp_len) { //Returns 0 if timeout or error, 1 if OK
    bool serverRetry = false;
    char url[PATH_MAX];
    char activation_token[PROXY_MAX_ACTIVATION_TOKEN_SIZE];
    char outBuf[PROXY_MAX_MSG_LEN];
    char response[PROXY_MAX_HTTP_SEND_MESSAGE_LEN];
    int retries = 0;
    http_param_t params;

    memset(&params, 0 , sizeof(params));

    params.timeouts.connectTimeout = DEFAULT_HTTP_CONNECT_TIMEOUT_SEC;
    params.timeouts.transferTimeout = DEFAULT_HTTP_TRANSFER_TIMEOUT_SEC;
    params.verbose = false;

    do {
        if (serverRetry == true) {
            retries++;
            sleep(1);
        }

        pc_getCloudURL(url, sizeof(url));

        pu_log(LL_INFO, "pt_http_write(): POST URL = %s", url);

        memset(outBuf, 0, sizeof(outBuf));
        memcpy(outBuf, buf, len);           //libhttpcomm_sendMsg spoiling data :-(
        pc_getActivationToken(activation_token, sizeof(activation_token));
        if (libhttpcomm_sendMsg(curlWriteHandle, CURLOPT_POST, url,
                                pc_getSertificatePath(), activation_token, outBuf,
                                len, response, sizeof(response), params, NULL) == 0) {

            serverRetry = (strlen(response) == 0) || (strstr(response, "ERR") != NULL) || (strstr(response, "ACK") == NULL);

            if(!serverRetry)
                pu_log(LL_INFO, "Send to server SUCCESS");
            else
                pu_log(LL_ERROR, "Error sending to server. Retry # %d: %s", retries, response);

        }
        else {
            // Either the Internet or the server is down
            // If the Internet is down, buffer messages and do not lose data
            pu_log(LL_INFO, "Couldn't contact the server, retry to connect");
            retries = 0;
            serverRetry = true;
        }
    }
    while (serverRetry == true && retries < PROXY_MAX_HTTP_RETRIES);

    *resp = response;
    *resp_len = strlen(response)+1;
    return (serverRetry)?0:1;
}
////////////////////////////////////////////////////////////////////////////////////////////
//get_command_id - Get command ID from json message
//json_string   - pointer to json string
//Return command ID or 0 if error
static unsigned long get_command_id(const char* json_string) {

    cJSON* val = cJSON_Parse(json_string);
    if(!val) {
        pu_log(LL_ERROR, "get_command_id: Can\'t parse server command %s", json_string);
        return 0;
    }
    cJSON* cmd_arr = cJSON_GetObjectItem(val, "commands");
    if(!cmd_arr) {
        pu_log(LL_ERROR, "get_command_id: Can\'t get commands list from server command %s", json_string);
        cJSON_Delete(val);
        return 0;
    }
    int cmd_arr_size = cJSON_GetArraySize(cmd_arr);
    if(!cmd_arr_size) {
        pu_log(LL_ERROR, "get_command_id: empty commands list from server command %s", json_string);
        cJSON_Delete(val);
        return 0;
    }
    if(cmd_arr_size > 1) {
        pu_log(LL_ERROR, "get_command_id: More than one command in command list. Answer for the first only! Message from server: %s", json_string);
    }

    cJSON * cmd_body = cJSON_GetArrayItem(cmd_arr, 0);
    if(!cmd_body) {
        pu_log(LL_ERROR, "get_command_id: wrong command object from server command %s", json_string);
        cJSON_Delete(val);
        return 0;
    }
    unsigned long command_id = cJSON_GetObjectItem(cmd_body,"commandId")->valueint;
    cJSON_Delete(val);

    return command_id;
}
////////////////////////////////////////////////////////////////////////////////////////////
//make_json_answer - create immediate reply to the server
//buf       - to this pointer the created responce will be copied
//buf_len   - buf capacity
//command_id- command id retrievad from the server's command
static void make_json_answer(char* buf, unsigned int buf_len, unsigned long command_id) {
// json_answer: "{"proxyId": <PROXY_ID>, "sequenceNumber": 1, "responses": [{"commandId": <command_id> "result": 0}]}";
    char da[4097];
    cJSON *root,*arr, *el0;
    root=cJSON_CreateObject();

    pc_getDeviceAddress(da, sizeof(da));
    cJSON_AddStringToObject(root, "proxyId", da);

    cJSON_AddNumberToObject(root,"sequenceNumber", 1);
    cJSON_AddItemToObject(root, "responses", arr=cJSON_CreateArray());
    el0=cJSON_CreateObject();
    cJSON_AddNumberToObject(el0,"commandId", command_id);
    cJSON_AddNumberToObject(el0,"result", 0);
    cJSON_AddItemToArray(arr, el0);

    char* res = cJSON_PrintUnformatted(root);

    strncpy(buf, res, buf_len);
    free(res);
    cJSON_Delete(root);
}
////////////////////////////////////////////////////////////////////////////////////////////
//send_responce_to_command - send immediate reply to the server
//command_id    - command ID extracted from the server's message
static void send_responce_to_command(unsigned long command_id) {
    bool serverRetry = false;
    char url[PATH_MAX];
    char outBuf[PROXY_MAX_MSG_LEN];
    char response[PROXY_MAX_HTTP_SEND_MESSAGE_LEN];
    char activation_token[PROXY_MAX_ACTIVATION_TOKEN_SIZE];
    int retries = 0;
    http_param_t params;

    memset(&params, 0 , sizeof(params));

    params.timeouts.connectTimeout = DEFAULT_HTTP_CONNECT_TIMEOUT_SEC;
    params.timeouts.transferTimeout = DEFAULT_HTTP_TRANSFER_TIMEOUT_SEC;
    params.verbose = false;

    do {
        if (serverRetry == true) {
            retries++;
            sleep(1);
        }

        pc_getCloudURL(url, sizeof(url));
        snprintf(url, sizeof(url)-strlen(url), "?id=");
        pc_getDeviceAddress(url+strlen(url), sizeof(url)-strlen(url));
        snprintf(url, sizeof(url)-strlen(url), "&timeout=%lu", params.timeouts.transferTimeout);

        pu_log(LL_DEBUG, "send_responce_to_command(): POST URL = %s", url);

        memset(response, 0, sizeof(response));
        memset(outBuf, 0, sizeof(outBuf));
        make_json_answer(outBuf, sizeof(outBuf), command_id);
        pc_getActivationToken(activation_token, sizeof(activation_token));
        if(libhttpcomm_postMsg(curlReadHandle, CURLOPT_POST, url, pc_getSertificatePath(), activation_token,
                               outBuf, strlen(outBuf), response, sizeof(response), params, NULL) == 0) {

            serverRetry = (strlen(response) == 0) || (strstr(response, "ERR") != NULL) || (strstr(response, "ACK") == NULL);

            if(!serverRetry)
                pu_log(LL_DEBUG, "send_responce_to_command(): Send to server SUCCESS");
            else
                pu_log(LL_ERROR, "send_responce_to_command(): Error sending to server. Retry # %d: %s", retries, response);

        }
        else {
            // Either the Internet or the server is down
            pu_log(LL_INFO, "send_responce_to_command(): Couldn't contact the server.");
            retries = 0;
            serverRetry = true;
        }
    }
    while (serverRetry == true && retries < PROXY_MAX_HTTP_RETRIES);
}
