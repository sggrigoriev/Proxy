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
    Created by gsg on 28/01/17.
*/

#include <string.h>
#include <assert.h>


#include "cJSON.h"

#include "pu_logger.h"
#include "ph_manager.h"

#include "pf_traffic_proc.h"
#include "pc_settings.h"
#include "pf_proxy_commands.h"


static const char* CLOUD_COMMANDS = "commands";
static const char* CMD_RESP_HD = "responses";
static const char* CMD_CMD_ID = "commandId";
static const char* CMD_RC = "result";

/* Make answer from the message and put into buf. Returns buf addess */
const char* pf_answer_to_command(cJSON* root, char* buf, size_t buf_size) {
/* json_answer: "{"responses": [{"commandId": <command_id> "result": 0}]}"; */
    assert(root); assert(buf); assert(buf_size);
    cJSON* arr = cJSON_GetObjectItem(root, CLOUD_COMMANDS);
    buf[0] = '\0';
    if(!arr) {
        return buf;
    }
    cJSON* resp_obj = cJSON_CreateObject();
    cJSON* resp_arr = cJSON_CreateArray();
    cJSON_AddItemToObject(resp_obj, CMD_RESP_HD, resp_arr);

    unsigned int i;
    for(i = 0; i < cJSON_GetArraySize(arr); i++) {
        cJSON* arr_item = cJSON_GetArrayItem(arr, i);
        cJSON* cmd_id = cJSON_GetObjectItem(arr_item, CMD_CMD_ID);
        if(!cmd_id) {
            pu_log(LL_ERROR, "%s item is not found in %d command item", CMD_CMD_ID, i);
            return buf;
        }
        cJSON* el = cJSON_CreateObject();

        cJSON_AddItemReferenceToObject(el, CMD_CMD_ID, cmd_id);
        cJSON_AddItemToObject(el, CMD_RC, cJSON_CreateNumber(0));
        cJSON_AddItemToArray(resp_arr, el);
    }

    char* res = cJSON_PrintUnformatted(resp_obj); /* Encoding responses array into string */

    strncpy(buf, res, buf_size-1);
    free(res);
    cJSON_Delete(resp_obj);

    return buf;
}

void pf_reconnect(pu_queue_t* to_proxy_main) {
    char buf[LIB_HTTP_MAX_MSG_SIZE];
    char device_id[LIB_HTTP_DEVICE_ID_SIZE];
    char conn_string[LIB_HTTP_MAX_URL_SIZE];
    char auth_token[LIB_HTTP_AUTHENTICATION_STRING_SIZE];
    char fw_version[LIB_HTTP_FW_VERSION_SIZE];

    pc_getProxyDeviceID(device_id, sizeof(device_id));

/* Send off-line status to the proxy_main */
    pr_make_cloud_off_cmd(buf, sizeof(buf), device_id);
    pu_queue_push(to_proxy_main, buf, strlen(buf)+1);

/* Make the reconnection */
    ph_reconnect();

/* Sent on-line status to the proxy main */
    pc_getCloudURL(conn_string, sizeof(conn_string));
    pc_getAuthToken(auth_token, sizeof(auth_token));
    pc_getFWVersion(fw_version, sizeof(fw_version));

    pr_make_conn_info_cmd(buf, sizeof(buf), conn_string, device_id, auth_token, fw_version);
    pu_queue_push(to_proxy_main, buf, strlen(buf)+1);
}

