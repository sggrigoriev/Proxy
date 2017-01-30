//
// Created by gsg on 28/01/17.
//

#include <string.h>

#include "cJSON.h"

#include "pu_logger.h"
#include "pc_settings.h"
#include "lib_http.h"
#include "ph_manager.h"
#include "pf_reboot.h"

#include "pf_proxy_commands.h"

static const char* HDR_VERSION = "version";
static const char* HDR_STATUS = "status";

static const char* CLOUD_COMMANDS = "commands";
static const char* CMD_DEVICE_ID = "deviceId";
static const char* CMD_PARAMETERS = "parameters";
static const char* CMD_PAR_NAME = "name";
static const char* CMD_PAR_NAME_VAL = "cloud";
static const char* CMD_PAR_VALUE = "value";

static const char* CMD_RESP_HD = "responses";
static const char* CMD_CMD_ID = "commandId";
static const char* CMD_RC = "result";


static cJSON* get_header(cJSON* root);
static void split_arrays(cJSON* hdr, cJSON* arr, cJSON** agent, cJSON** proxy);

static const char* getUrlParam(cJSON* cmd, int idx);
static const char* get_device_id(cJSON* item);


//Make 3 JSOn objects: header, Proxy commands array; Agent commands array
//NB! Obgects are detached from the main and should all be deleted!
//Return NULL if no commands found
pf_cmd_t* pf_parse_cloud_commands(const char* cloud_msg) {
    pf_cmd_t* ret = malloc(sizeof(pf_cmd_t));
    if(!ret) {
        pu_log(LL_ERROR, "pf_parse_cloud_command: memory allocation error. Reboot");
        pf_reboot();
    }
    if(ret->obj = cJSON_Parse(cloud_msg), !ret->obj) {
        pu_log(LL_ERROR, "pf_parse_cloud_command: Error cloud's commands message parsing:%s", cloud_msg);
        goto on_err;
    }
    if(ret->header = get_header(ret->obj), !ret->header) goto on_err;
    split_arrays(ret->obj, cJSON_GetObjectItem(ret->obj, CLOUD_COMMANDS), &ret->agent_cmd_array, &ret->proxy_cmd_array);

    return ret;
on_err:
    pf_close_cloud_commands(ret);
    return NULL;
}
void pf_close_cloud_commands(pf_cmd_t* cmd) {
    if(cmd) {
        if(cmd->obj) cJSON_Delete(cmd->obj);
        if(cmd->header) cJSON_Delete(cmd->header);
        if(cmd->proxy_cmd_array) cJSON_Delete(cmd->proxy_cmd_array);
        if(cmd->agent_cmd_array) cJSON_Delete(cmd->agent_cmd_array);
        free(cmd);
    }
}

void pf_process_proxy_commands(pf_cmd_t* cmd) { //NB! currntly we got just one command: update main URL
    if((!cmd) || (!cmd->proxy_cmd_array)) return;
    const char* main_url = getUrlParam(cmd->proxy_cmd_array, 0);
    if(!strlen(main_url)) {
        pu_log(LL_ERROR, "pf_process_proxy_commands: Main URL string is empty. Ignored");
        return;
    }
    if(!ph_update_main_url(main_url)) {
        pu_log(LL_ERROR, "pf_process_proxy_commands: Main URL update failed");
    }
}

void pf_encode_agent_commands(pf_cmd_t* cmd, char* resp, size_t size) {
    resp[0] = '\0';
    if((!cmd) || (!cmd->agent_cmd_array) || (!cmd->header)) return;
    cJSON_AddItemToObject(cmd->header, CLOUD_COMMANDS, cmd->agent_cmd_array);
    char* result = cJSON_PrintUnformatted(cmd->header);
    strncpy(resp, result, size-1);
    resp[size-1] = '\0';
    free(result);
}

//Make answer from the message and put into buf. Returns buf addess
const char* pf_answer_to_command(cJSON* root, char* buf, size_t buf_size) {
// json_answer: "{"responses": [{"commandId": <command_id> "result": 0}]}";
    cJSON* arr = cJSON_GetObjectItem(root, CLOUD_COMMANDS);
    buf[0] = '\0';
    if(!arr) {
        return buf;
    }
    cJSON* cmds = cJSON_CreateObject();
    cJSON* resp_arr = cJSON_CreateArray();
    cJSON_AddItemToObject(cmds, CMD_RESP_HD, resp_arr);

    for(unsigned int i = 0; i < cJSON_GetArraySize(arr); i++) {
        cJSON* el = cJSON_CreateObject();
        cJSON* rc = cJSON_CreateObject();
        cJSON_AddItemToObject(rc, CMD_RC, cJSON_CreateNumber(0));

        cJSON_AddItemReferenceToObject(el, CMD_CMD_ID, cJSON_GetObjectItem(cJSON_GetArrayItem(arr, i), CMD_CMD_ID));
        cJSON_AddItemToObject(el, CMD_RC, rc);
        cJSON_AddItemToArray(resp_arr, el);
    }

    char* res = cJSON_PrintUnformatted(root);

    strncpy(buf, res, buf_size-1);
    free(res);
    cJSON_Delete(cmds);
    return buf;
}

static cJSON* get_header(cJSON* root) {
    cJSON* ret = cJSON_CreateObject();
    if(!ret) return NULL;
    cJSON_AddItemReferenceToObject(ret, HDR_VERSION, cJSON_GetObjectItem(root, HDR_VERSION));
    cJSON_AddItemReferenceToObject(ret, HDR_STATUS, cJSON_GetObjectItem(root, HDR_STATUS));
    return ret;
}

static void split_arrays(cJSON* hdr, cJSON* arr, cJSON** agent, cJSON** proxy) {
    char device_id[LIB_HTTP_DEVICE_ID_SIZE];
    pc_getProxyDeviceID(device_id, sizeof(device_id));

    *agent = NULL; *proxy = NULL;
    if(!arr) return;
    for(unsigned int i = 0; i < cJSON_GetArraySize(arr); i++) {
        cJSON* item = cJSON_GetArrayItem(arr, i);
        if(!item) {
            pu_log(LL_ERROR, "Error extratcing %d command from commands array. Skip", i);
            continue;
        }
        cJSON* par_array = cJSON_GetObjectItem(item, CMD_PARAMETERS);
        if(!par_array) {
            pu_log(LL_ERROR, "Error extratcing %d command's item %s from commands array. Skip", i, CMD_PARAMETERS);
            continue;
        }
        if(!cJSON_GetArraySize(par_array)) {
            pu_log(LL_ERROR, "Error extratcing %d command's item %s from commands array. Skip", i, CMD_PARAMETERS);
            continue;
        }
        cJSON* par_item = cJSON_GetArrayItem(arr, 0);   //Check the name of the first param - the command could be for Proxy - Command which defines by its parameters - amaizing way to make others fucking hard...
        cJSON* par_name = cJSON_GetObjectItem(par_item, CMD_PAR_NAME);
        if(!par_name) {
            pu_log(LL_ERROR, "Error extratcing %d command's 1st param name from commands array. Skip", i);
            continue;
        }
        if(!strcmp(par_name->valuestring, CMD_PAR_NAME_VAL) && !strcmp(get_device_id(hdr), device_id)) {  //Bingo! We got the Proxy's command!
            if(!(*proxy)) *proxy = cJSON_CreateArray();
            cJSON_AddItemReferenceToArray(*proxy, item);
        }
        else {      //agent's command
            if(!(*agent)) *agent = cJSON_CreateArray();
            cJSON_AddItemReferenceToArray(*agent, item);
        }
    }
}


static const char* get_device_id(cJSON* item) {
    cJSON* d_id = cJSON_GetObjectItem(item, CMD_DEVICE_ID);
    if(!d_id) return "";
    if(d_id->type != cJSON_String) return "";
    return d_id->valuestring;
}

static const char* getUrlParam(cJSON* cmd, int idx) {
    cJSON* arr = cJSON_GetObjectItem(cmd, CMD_PARAMETERS);
    if(!arr) return "";
    if(cJSON_GetArraySize(arr) != 1) return "";
    cJSON* item = cJSON_GetArrayItem(arr, idx);
    if(!item) return "";
    cJSON* pname = cJSON_GetObjectItem(item, CMD_PAR_NAME);
    if((!pname) || (pname->type != cJSON_String)) return "";
    if(strcmp(pname->valuestring, CMD_PAR_NAME_VAL)) return "";

    cJSON* val = cJSON_GetObjectItem(item, CMD_PAR_VALUE);
    if((!val) || (val->type != cJSON_String)) return "";
    return val->valuestring;
}