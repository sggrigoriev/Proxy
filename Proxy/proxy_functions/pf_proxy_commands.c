//
// Created by gsg on 28/01/17.
//

#include <string.h>
#include <pf_traffic_proc.h>
#include <assert.h>

#include "cJSON.h"

#include "pu_logger.h"
#include "pc_settings.h"
#include "lib_http.h"

#include "pf_proxy_commands.h"


static const char* CLOUD_COMMANDS = "commands";
static const char* CMD_RESP_HD = "responses";
static const char* CMD_CMD_ID = "commandId";
static const char* CMD_RC = "result";

//Make answer from the message and put into buf. Returns buf addess
const char* pf_answer_to_command(cJSON* root, char* buf, size_t buf_size) {
// json_answer: "{"responses": [{"commandId": <command_id> "result": 0}]}";
    assert(root); assert(buf); assert(buf_size);
    cJSON* arr = cJSON_GetObjectItem(root, CLOUD_COMMANDS);
    buf[0] = '\0';
    if(!arr) {
        return buf;
    }
    cJSON* resp_obj = cJSON_CreateObject();
    cJSON* resp_arr = cJSON_CreateArray();
    cJSON_AddItemToObject(resp_obj, CMD_RESP_HD, resp_arr);

    for(unsigned int i = 0; i < cJSON_GetArraySize(arr); i++) {
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

    char* res = cJSON_PrintUnformatted(resp_obj); //Encoding responses array into string

    strncpy(buf, res, buf_size-1);
    free(res);
    cJSON_Delete(resp_obj);
    char device_id[LIB_HTTP_DEVICE_ID_SIZE];
    pc_getProxyDeviceID(device_id, sizeof(device_id));
    pf_add_proxy_head(buf, buf_size, device_id, 11037);

    return buf;
}

