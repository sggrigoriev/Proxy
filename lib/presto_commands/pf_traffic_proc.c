//
// Created by gsg on 13/12/16.
//

#include <stdio.h>
#include <string.h>
#include <cJSON.h>

#include "lib_http.h"
#include <pu_logger.h>
#include "pf_traffic_proc.h"

static unsigned long get_command_id(const char* json_string);

size_t pf_add_proxy_head(char* msg, size_t msg_size, const char* device_id, unsigned int seq_number) {
    char buf[LIB_HTTP_MAX_MSG_SIZE*2];
    unsigned int i = 0;
    while((msg[i++] != '{') && (i < strlen(msg)));

    if(i >= strlen(msg)) i = 0;

    snprintf(buf, sizeof(buf)-1, "{\"proxyId\": \"%s\", \"sequenceNumber\": %d, %s}", device_id, seq_number, msg+i);

    strncpy(msg, buf, msg_size-1);
    msg[msg_size-1] = '\0';

    return strlen(msg);
}
//return 1 if the mesagage contains command from the cloud
int pf_command_came(const char* msg) {
    return (strstr(msg, "command") != NULL);
}
//Make answer from the message and put into buf. Returns buf addess
const char* pf_answer_to_command(char* buf, size_t buf_size, const char* msg) {
// json_answer: "{"proxyId": <PROXY_ID>, "sequenceNumber": 1, "responses": [{"commandId": <command_id> "result": 0}]}";

    cJSON *root,*arr, *el0;
    root=cJSON_CreateObject();

    cJSON_AddItemToObject(root, "responses", arr=cJSON_CreateArray());
    el0=cJSON_CreateObject();
    cJSON_AddNumberToObject(el0,"commandId", get_command_id(msg));
    cJSON_AddNumberToObject(el0,"result", 0);
    cJSON_AddItemToArray(arr, el0);

    char* res = cJSON_PrintUnformatted(root);

    strncpy(buf, res, buf_size-1);
    free(res);
    cJSON_Delete(root);
    return buf;
}
////////////////////////////////////////////////
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
    unsigned long command_id = (unsigned long)cJSON_GetObjectItem(cmd_body,"commandId")->valueint;
    cJSON_Delete(val);

    return command_id;
}