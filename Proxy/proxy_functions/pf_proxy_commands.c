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


static const char* CLOUD_COMMANDS = "commands";
static const char* CMD_DEVICE_ID = "deviceId";
static const char* CMD_PARAMETERS = "parameters";
static const char* CMD_PAR_NAME = "name";
static const char* CMD_PAR_NAME_VAL = "cloud";
static const char* CMD_PAR_VALUE = "value";


static const char* getUrlParam(cJSON* cmd, int idx);
static const char* get_device_id(cJSON* item);


//Return NULL if it is not a command
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
    if(ret->cmd_array = cJSON_GetObjectItem(ret->obj, CLOUD_COMMANDS), !ret->cmd_array) { //this is not a command
       free(ret);
        return NULL;
    }
    bzero(ret->main_url, sizeof(ret->main_url));
    if(!cJSON_GetArraySize(ret->cmd_array)) {
        pu_log(LL_ERROR, "pf_parse_cloud_command: %s array is empty:%s", CLOUD_COMMANDS, cloud_msg);
        goto on_err;
    }

    char device_id[LIB_HTTP_DEVICE_ID_SIZE];
    pc_getProxyDeviceID(device_id, sizeof(device_id));

    ret->proxy_commands = 0;
    ret->total_commands = cJSON_GetArraySize(ret->cmd_array);

    for(unsigned int i = 0; i < ret->total_commands; i++) {
        if(!strncmp(get_device_id(cJSON_GetArrayItem(ret->cmd_array, i)), device_id, sizeof(device_id))) {  //This command about the proxy

            const char* url = getUrlParam(cJSON_GetArrayItem(ret->cmd_array, i), 0);
            if(strlen(url)) {
                strncpy(ret->main_url, url, sizeof(ret->main_url)-1);
                ret->proxy_commands = 1;
                pu_log(LL_DEBUG, "pf_parse_cloud_command: Main URL update came from the cloud:%s", cloud_msg);
                break;
            }
        }
    }
    return ret;
on_err:
    pf_close_cloud_commands(ret);
    return NULL;
}
void pf_close_cloud_commands(pf_cmd_t* cmd) {
    if(cmd) {
        if(cmd->obj) cJSON_Delete(cmd->obj);
        free(cmd);
    }
}

//Return 1 if there are some commands for the Proxy
int pf_are_proxy_commands(pf_cmd_t* cmd) {
    return (cmd != NULL) && (cmd->proxy_commands > 0);
}
//Return 1 if there are some commands for the Agent
int pf_are_agent_commands(pf_cmd_t* cmd) {
    return (cmd != NULL) && ((cmd->total_commands - cmd->proxy_commands) > 0);
}
void pf_process_proxy_commands(pf_cmd_t* cmd) {
    if((!cmd) || (!strlen(cmd->main_url))) return;
    if(!ph_update_main_url(cmd->main_url)) {
        pu_log(LL_ERROR, "pf_process_proxy_commands: Main URL update failed");
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