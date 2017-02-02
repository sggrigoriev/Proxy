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
#include "ph_manager.h"
#include "pf_reboot.h"

#include "pf_proxy_commands.h"

#define NOREF(a) (a&0xF)

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
static void split_arrays(cJSON* arr, cJSON** agent, cJSON** proxy);
//Return 1 if it is proxy command
static int is_proxy_cmd(cJSON* cmd_arr_item);

static const char* getUrlParam(cJSON* cmd_arr);


//Make 3 JSOn objects: header, Proxy commands array; Agent commands array
//NB! Obgects are detached from the main and should all be deleted!
//Return NULL if no commands found
pf_cmd_t* pf_parse_cloud_commands(const char* cloud_msg) {
    cJSON* root = cJSON_Parse(cloud_msg);
    if(!root) {
        pu_log(LL_ERROR, "pf_parse_cloud_command: Error cloud's message parsing:%s", cloud_msg);
        return NULL;
    }
    cJSON* cmd_array = cJSON_GetObjectItem(root, CLOUD_COMMANDS);
    if(!cmd_array) {    //this message is not a command!
        cJSON_Delete(root);
        return NULL;
    }
//"commands" item is found - let's work now
    pf_cmd_t* ret = malloc(sizeof(pf_cmd_t));
    if(!ret) {
        pu_log(LL_ERROR, "pf_parse_cloud_command: memory allocation error. Reboot");
        pf_reboot();
    }
    bzero(ret, sizeof(pf_cmd_t));
    ret->obj = root;
    if(ret->header = get_header(ret->obj), !ret->header) goto on_err;
    split_arrays(cmd_array, &ret->agent_cmd_array, &ret->proxy_cmd_array);
    if((!ret->agent_cmd_array) && (!ret->proxy_cmd_array)) { //this is not a command!
        pf_close_cloud_commands(ret);
        return NULL;
    }

    return ret;
on_err:
    pu_log(LL_ERROR, "pf_parse_cloud_command: error parsing %s", cloud_msg);
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
    cmd = NULL;
}

void pf_process_proxy_commands(pf_cmd_t* cmd) { //NB! currntly we got just one command: update main URL
    if((!cmd) || (!cmd->proxy_cmd_array)) return;
    const char* main_url = getUrlParam(cmd->proxy_cmd_array);
    if(!strlen(main_url)) return;

    if(!ph_update_main_url(main_url)) {
        pu_log(LL_ERROR, "pf_process_proxy_commands: Main URL update failed");
    }
}

void pf_encode_agent_commands(pf_cmd_t* cmd, char* resp, size_t size) {
    resp[0] = '\0';
    if((!cmd) || (!cmd->agent_cmd_array) || (!cmd->header)) return;
    cJSON_AddItemReferenceToObject(cmd->header, CLOUD_COMMANDS, cmd->agent_cmd_array);
    char* result = cJSON_PrintUnformatted(cmd->header);
    strncpy(resp, result, size-1);
    resp[size-1] = '\0';
    free(result);
}

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

static cJSON* get_header(cJSON* root) {
    cJSON* ret = cJSON_CreateObject();
    if(!ret) return NULL;
    cJSON* ver = cJSON_GetObjectItem(root, HDR_VERSION);
    if(!ver) {
        pu_log(LL_ERROR, "get_header: %s is not found in commands header", HDR_VERSION);
        return NULL;
    }
    cJSON* stat = cJSON_GetObjectItem(root, HDR_STATUS);
    if(!stat) {
        pu_log(LL_ERROR, "get_header: %s is not found in commands header", HDR_STATUS);
        return NULL;
    }
    cJSON_AddItemReferenceToObject(ret, HDR_VERSION, ver);
    cJSON_AddItemReferenceToObject(ret, HDR_STATUS, stat);
    return ret;
}

static void split_arrays(cJSON* arr, cJSON** agent, cJSON** proxy) {
    assert(arr);

    *agent = NULL; *proxy = NULL;

    for(unsigned int i = 0; i < cJSON_GetArraySize(arr); i++) {
        cJSON *item = cJSON_GetArrayItem(arr, i);
        if (!item) {
            pu_log(LL_ERROR, "Error extratcing %d command from commands array. Skip", i);
            continue;
        }
        if (is_proxy_cmd(item)) {
            if (!(*proxy)) *proxy = cJSON_CreateArray();
            cJSON_AddItemReferenceToArray(*proxy, item);
        }
        else {
            if(!(*agent)) *agent = cJSON_CreateArray();
            cJSON_AddItemReferenceToArray(*agent, item);
        }
    }
}
//Return 1 if it is proxy command
static int is_proxy_cmd(cJSON* cmd_arr_item) {
    cJSON* par_array = cJSON_GetObjectItem(cmd_arr_item, CMD_PARAMETERS);
    if(!par_array) {
        pu_log(LL_WARNING, "Error extratcing parameters for command's item %s from commands array.", CMD_PARAMETERS);
        return 0;
    }
    if(!cJSON_GetArraySize(par_array)) {
        pu_log(LL_WARNING, "Parameters array %s is empty.", CMD_PARAMETERS);
        return 0;
    }
    cJSON* par_item = cJSON_GetArrayItem(par_array, 0);   //Check the name of the first param - the command could be for Proxy - Command which defines by its parameters - amaizing way to make others fucking hard...
    if(!par_item) {
        pu_log(LL_WARNING, "Parameters array 1st element is not found");
        return 0;
    }
    cJSON* par_name = cJSON_GetObjectItem(par_item, CMD_PAR_NAME);
    if(!par_name) {
        pu_log(LL_WARNING, "Parameter item %s is not found", CMD_PAR_NAME);
        return 0;
    }
    if(NOREF(par_name->type) != cJSON_String) {
        pu_log(LL_WARNING, "Parameter item %s is not a string", CMD_PAR_NAME);
        return 0;
    }
    if(strcmp(par_name->valuestring, CMD_PAR_NAME_VAL)) return 0;   //this is not a Proxy command - "cloud" name dosen't match
//parameter found. Let's check the device id - maybe this command is not for our Proxy?
    cJSON* d_id = cJSON_GetObjectItem(cmd_arr_item, CMD_DEVICE_ID);
    if(!d_id) {
        pu_log(LL_WARNING, "Parameter item %s is not found", CMD_DEVICE_ID);
        return 0;
    }
    if(NOREF(d_id->type) != cJSON_String) {
        pu_log(LL_WARNING, "Parameter item %s should has a string type", CMD_DEVICE_ID);
        return 0;
    }
    char device_id[LIB_HTTP_DEVICE_ID_SIZE];
    pc_getProxyDeviceID(device_id, sizeof(device_id));
    if(strcmp(d_id->valuestring, device_id)) return 0;  //Different deviceID

    return 1;   //We got it! (finally)
}

static const char* getUrlParam(cJSON* cmd_arr) {
    for(unsigned int i = 0; i < cJSON_GetArraySize(cmd_arr); i++) {
        cJSON* params_arr = cJSON_GetArrayItem(cmd_arr, i);
        for(unsigned int j = 0; j < cJSON_GetArraySize(params_arr); j++) {
            cJSON* param_item = cJSON_GetArrayItem(params_arr, j);
            if(!param_item) return "";
            cJSON* param_name = cJSON_GetObjectItem(param_item, CMD_PAR_NAME);
            if(!param_name) return "";
            if(strcmp(param_name->valuestring, CMD_PAR_NAME_VAL)) continue;  //skip the parameter
            cJSON* val = cJSON_GetObjectItem(param_item, CMD_PAR_VALUE);
            if(!val) {
                pu_log(LL_ERROR, "Main URL update: parameter %s value %s is not found", CMD_PAR_NAME, CMD_PAR_VALUE);
                return "";
            }
            if(NOREF(val->type) != cJSON_String) {
                pu_log(LL_ERROR, "Main URL update: parameter %s value %s is not a string", CMD_PAR_NAME, CMD_PAR_VALUE);
                return "";
            }
            return val->valuestring;
        }
    }
    pu_log(LL_ERROR, "Main URL update: connection string is not found");
    return "";
}