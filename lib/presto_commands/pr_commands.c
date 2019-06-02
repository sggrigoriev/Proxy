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
 *

 Created by gsg on 08/12/16.

 Interface for all cloud <-> gateway commands parsing and creation. Currently looks like the garbage can. Also contains some helpers.
*/
#include <string.h>
#include <malloc.h>
#include <assert.h>
#include <pthread.h>

#include "pu_logger.h"
#include "pr_commands.h"

/*
 the strucure: {"alerts":[{"alertId":<seq#>,"deviceId":"<proxyID>","alertType":"CCC-NNN","timestamp":<NNNNNNN>,"paramsMap"{...}}]}
header items:
*/

static const char* commands = "commands";
static const char* deviceId = "deviceId";
/*common items:*/
static const char* alertType = "alertType";
/*static const char* cmd_type = "type"; */
/*static const char* timestamp = "timestamp";*/
static const char* paramsMap = "paramsMap";
static const char* paramsArray = "parameters";
static const char* cmd_par_name = "name";
static const char* cmd_par_value = "value";
/*all WUD alerts */
static const char* alerts = "alerts";
static const char* alertText = "alertText";
static const char* component = "component";
static const char* wud_ping = "wud_ping";       /*special case, blin... */
/*
item names for cmd
*/
static const char* cmd_file_server_url = "firmwareUrl";
static const char* cmd_fw_update_check_sum = "firmwareCheckSum";
static const char* cmd_fw_update_status = "firmwareUpdateStatus";

static const char* cmd_main_url_cloud = "cloud";

static const char* cmd_restart = "restartChild";

static const char* cmd_conn_string = "connString";
static const char* cmd_stop = "shutYouselfDown";

static const char* cmd_device_id = "deviceId";
static const char* cmd_auth_token = "authToken";
static const char* cmd_firmware = "firmware";

static const char* cmd_cloud_off = "cloudOff";

static const char* cmd_reboot = "reboot";

/* Agent commands/requests */
static const char* AGENT_REQUEST = "agent_request";
static const char* AGENT_CONNECT = "connect";

/*Using for PROC_NAMES changes protecion only */
static pthread_mutex_t own_mutex = PTHREAD_MUTEX_INITIALIZER;

static char PROC_NAMES[PR_CHILD_SIZE][PR_MAX_PROC_NAME_SIZE] = {{0}};

void pr_store_child_name(int child_name, const char* name) {
    if(!name || !pr_child_t_range_check(child_name)) return;
    pthread_mutex_lock(&own_mutex);
        strncpy(PROC_NAMES[child_name], name, PR_MAX_PROC_NAME_SIZE-1);
    pthread_mutex_unlock(&own_mutex);
}

const char* pr_chld_2_string(pr_child_t child_name) {
    if(!pr_child_t_range_check(child_name)) {
        return "";
    }
    return PROC_NAMES[child_name];
}

pr_child_t pr_string_2_chld(const char* child_name) {
    pr_child_t ret = PR_CHILD_SIZE;
    unsigned int i;
    for(i = 0; i < PR_CHILD_SIZE; i++) {
        if(!strcmp(child_name, PROC_NAMES[i])) {
            ret = (pr_child_t)i;
            break;
        }
    }
    return ret;
}

int pr_child_t_range_check(int item) {
    return (item >= 0) && (item < PR_CHILD_SIZE);
}

/*
Messaging part
local functions for messaging
*/
/*
    Recognize the command type by parameters array attached to the command
    params_array    - parsed parameters part of a command
    Return command type or PR_CMD_UNDEFINED
*/
static pr_cmd_item_t get_cmd_params_from_array(cJSON* params_array);

/*
    Recognize the connad type by the parameters map, attached to a command. Currently some commands have its params as an array and some - as a map.
    params_map      - parsed parameters part of a command
    Return command type or PR_CMD_UNDEFINED
*/
static pr_cmd_item_t get_cmd_params_from_map(cJSON* params_map);

/*Compare de device id stoerd in item aith device_id.
  item        - message item
  device_id   - gateway device id
Return 1 if compared. 0 if not
*/
static int cmp_device_id(cJSON* item, const char* device_id);

/*Add the referenced item to array. Create array if it is NULL
  array   - array, containgng reference items (NULL if no items)
  ref     - added reference item
Return the chaned array
*/
static cJSON* add_ref2array(cJSON* array, cJSON* ref);

/*Get name, value pair from the command item. Command parameters array element has the structure "name":"<name>, "value":"<value>"
  cmd_item    - command's parameters array element
  name        - returned parameter name pointer
  value       - retured parameter value pointer
Return 1 if OK, 0 if the item does not contain such a pair
*/
static int get_name_value_pair(cJSON* cmd_item, char** name, char** value);

/*Get the value by its name from the item
  cmd_item    - item containing  the info
  name        - parameter name
Return poiner to the value ir NULL if the cmd_item does not contain the parameter with this name
*/
static char* get_parameter(cJSON* cmd_item, const char* name);

/*
Command functions

NB! Commented string could used in testing puposes only: no thread protection on JSON static data!
Returns NULL if bad
*/
msg_obj_t* pr_parse_msg(char* msg) {
    msg_obj_t* ret = cJSON_Parse(msg);
/*
    if(!ret) {
        pu_log(LL_ERROR, "Error parsing %s from here: %s", msg, cJSON_GetErrorPtr());
    }
*/
    return ret;
}

void pr_erase_msg(msg_obj_t* msg) {
    if(msg) cJSON_Delete(msg);
}

size_t pr_get_array_size(msg_obj_t* array) {
    if(!array) return 0;
    if(array->type != cJSON_Array) return 0;
    return (size_t)cJSON_GetArraySize(array);
}

msg_obj_t* pr_get_arr_item(msg_obj_t* array, size_t idx) {
    if(!array || (array->type != cJSON_Array) || (idx > (size_t)cJSON_GetArraySize(array))) return NULL;
    return cJSON_GetArrayItem(array, (int)idx);
}

/*Return obj_msg, converted to plain string*/
void  pr_obj2char(msg_obj_t* obj_msg, char* text_msg, size_t size) {
    if(!obj_msg) text_msg[0] = '\0';
    char* str = cJSON_PrintUnformatted(obj_msg);
    strncpy(text_msg, str, size-1);
    text_msg[size-1] = '\0';
    free(str);
}

msg_obj_t* pr_get_cmd_array(msg_obj_t* obj) {
    return cJSON_GetObjectItem(obj, commands);
}

msg_obj_t* pr_make_cmd_array(msg_obj_t* cmd_arr_elem) {
    cJSON* ret = cJSON_CreateObject();
    cJSON* arr = cJSON_CreateArray();
    cJSON_AddItemReferenceToArray(arr, cmd_arr_elem);
    cJSON_AddItemToObject(ret, commands, arr);
    return ret;
}

msg_obj_t* pr_get_alerts_array(msg_obj_t* obj) {
    cJSON* ret = cJSON_GetObjectItem(obj, alerts);
    if(ret) return ret;
    return cJSON_GetObjectItem(obj, wud_ping);
}

/*
PR_CMD_UNDEFINED,
COMMON HEAD: { "type": <number>, "commandId": "<number>", "deviceId": "<GW_ID>", ...}
*PR_CMD_FWU_START "parameters": [
    {"name": "firmwareUpdateStatus","value": "1"},{"name": "firmwareUrl","value": "<url>"},{"name": "firmwareCheckSum","value": "<check_sum>"}, ]}
*PR_CMD_FWU_CANCEL "parameters": [{"name": "firmwareUpdateStatus","value": "0"}]
*PR_CMD_CLOUD_CONN "parameters": [{"name":"connString", "value": "<url>"}, {"name":"deviceId", "value": "<device_id>"},
    {"name":"authToken", value": "<authToken>"}, {"name": "firmware", "value": "<version>"}]

PR_CMD_RESTART_CHILD "paramsMap": {"restartChild": "<child_name>"}
PR_CMD_STOP "paramsMap": {"shutYouselfDown": "1"}
PR_CMD_UPDATE_MAIN_URL: "paramsMap": {"cloud": "url"}
 */
pr_cmd_item_t pr_get_cmd_item(msg_obj_t* cmd_item) {
    pr_cmd_item_t ret;
    ret.command_type = PR_CMD_UNDEFINED;

    cJSON* params = cJSON_GetObjectItem(cmd_item, paramsArray);
    if(params)  return get_cmd_params_from_array(params);

    params = cJSON_GetObjectItem(cmd_item, paramsMap);
    if(params) return get_cmd_params_from_map(params);

    return ret;
}

/*Return commands array for Proxy or "", return full message w/o proxy commands for Agent or ""*/
void pr_split_msg(msg_obj_t* msg, const char* device_id, char* msg4proxy, size_t msg4proxy_size, char* msg4agent, size_t msg4agent_size) {
    msg4proxy[0] = '\0';
    msg4agent[0] = '\0';

    cJSON* array = cJSON_GetObjectItem(msg, commands);
    if(!array) {
        pr_obj2char(msg, msg4agent, msg4agent_size);
        return;
    }
/*Got commands. Let split'em on reds and whites*/
    cJSON* agent_cmd = NULL;
    cJSON* proxy_cmd = NULL;
    unsigned int i;
    for(i = 0; i < cJSON_GetArraySize(array); i++) {
        cJSON *item = cJSON_GetArrayItem(array, i);
        if (!item) {
            pu_log(LL_ERROR, "Error extracting %d command from commands array. Skip", i);
            continue;
        }
        if(!cmp_device_id(item, device_id)) {               /* Alien deviceID - Agent's busines */
            agent_cmd = add_ref2array(agent_cmd, item);
        }
        else {                                             /* There is a command for paring with Proxy deviceID, so we need to recognize the command itself */
            pr_cmd_item_t cmd = pr_get_cmd_item(item);
            if (cmd.command_type != PR_CMD_UNDEFINED)
                proxy_cmd = add_ref2array(proxy_cmd, item); /* Proxy command */
            else
                agent_cmd = add_ref2array(agent_cmd, item); /* Proxy hash't such a command, so let Agent to work with it */
        }
    }
    if(agent_cmd) {
        cJSON* agent_with_head = cJSON_CreateObject();                  /*Adding "commands" to the head and let it go*/
        cJSON_AddItemToObject(agent_with_head, commands, agent_cmd);
        pr_obj2char(agent_with_head, msg4agent, msg4agent_size);
        cJSON_Delete(agent_with_head);
    }
    if(proxy_cmd) {
        pr_obj2char(proxy_cmd, msg4proxy, msg4proxy_size);
        cJSON_Delete(proxy_cmd);
    }
}

/*Retrieve from msg_object message type*/
pr_msg_type_t pr_get_message_type(msg_obj_t* msg) {
    cJSON* obj = cJSON_GetObjectItem(msg, commands);
    if(obj) return PR_COMMANDS_MSG;
    obj = cJSON_GetObjectItem(msg, alerts);
    if(obj) return PR_ALERTS_MSG;
    obj = cJSON_GetObjectItem(msg, wud_ping);
    if(obj) return PR_ALERTS_MSG;
    return PR_OTHER_MSG;
}

/*/
Commands creation
*/
/*
{"commands": [
    {"type": 1, "commandId": "11038", "deviceId": "<proxy device id>", "parameters": [
        {"name": "connString", "value": "<URL>"}, {"name": "deviceId", "value": "<proxy device id>"},
        {"name": "authToken", "value": "<auth_token>"}, {"name": "firmware", "value": "<fw fersion">}]
    }
]}
 */
const char* pr_make_conn_info_cmd(char* buf, size_t size, const char* conn_string, const char* device_id, const char* auth_token, const char* version) {
    const char* head1 = "{\"commands\": [{\"type\": 1, \"commandId\": \"11038\", \"deviceId\": \"";
    const char* head2 = "\",";
    const char* part1 = "\"parameters\": [{\"name\":\"connString\", \"value\": \""; 
    const char* part2 = "\"}, {\"name\":\"deviceId\", \"value\":\"";  
    const char* part3 = "\"}, {\"name\":\"authToken\", \"value\": \"";    
    const char* part4 = "\"}, {\"name\":\"firmware\", \"value\": \"";   
    const char* part5 = "\"}]}]}";

    snprintf(buf, size-1, "%s%s%s%s%s%s%s%s%s%s%s%s", head1, device_id, head2, part1, conn_string, part2, device_id, part3, auth_token, part4, version, part5);
    return buf;
}
/*
{"commands": [
    {"type": 1, "commandId": "11038", "deviceId": "<gw device id>", "parameters": [{"name": "cloudOff", "value": "1"}]}
]}
 */
const char* pr_make_cloud_off_cmd(char* buf, size_t size, const char* device_id) {
    const char* head1 = "{\"commands\": [{\"type\": 1, \"commandId\": \"11038\", \"deviceId\": \"";
    const char* head2 = "\",";
    const char* part1 = "\"parameters\": [{\"name\":\"";
    const char* part2 = "\", \"value\": \"1\"}]}]}";

    snprintf(buf, size-1, "%s%s%s%s%s%s", head1, device_id, head2, part1, cmd_cloud_off, part2);
    return buf;
}

/*
{"commands": [
    {"type": 1, "commandId": "11038", "deviceId": "<gw device id>", "paramsMap": {"restartChild": "<child_name>"}}
]}
 */
const char* pr_make_restart_child_cmd(char* buf, size_t size, const char* child_name) {
    const char* head1 = "{\"commands\": [{ \"type\": 1, \"commandId\": \"11038\", \"deviceId\": \"";
    const char* head2 = "\",";
    const char* part1 = "\"paramsMap\": {\"restartChild\": \"";  
    const char* part2 = "\"}}]}";

    snprintf(buf, size-1, "%s%s%s%s%s%s", head1, "the best device id!", head2, part1, child_name, part2);
    return buf;
}
/*
/
Alerts functions
*/
/*
{"measures": [{"deviceId": "<gw device id>", "params": [
    {"name": "firmware", "value": "<fw version>"},
    {"name": "firmwareUpdateStatus": "<0 - stop/1 - start/2 - in process>"}]
}
 */
const char* pr_make_fw_status4cloud(char* buf, size_t size, fwu_status_t status, const char* fw_version, const char* device_id) {
    const char* part1 = "{\"measures\": [{\"deviceId\": \""; 
    const char* part2 = "\",\"params\": [{\"name\": \"firmware\", \"value\": \""; 
    const char* part3 = "\"},{\"name\": \"firmwareUpdateStatus\", \"value\": \"";
    const char* part4 = "\"}]}]}";

    snprintf(buf, size-1, "%s%s%s%s%s%d%s", part1, device_id, part2, fw_version, part3, status, part4);
    return buf;
}
/*
 * {"measures": [{"deviceId": "Aiox-11038", "params": [{"name": "ipAddress", "value": "<local IP address>}]}]}
 */
const char* pr_make_local_ip_notification(char* buf, size_t size, const char* local_ip_address, const char* device_id) {
    const char* part1 = "{\"measures\": [{\"deviceId\": \"";
    const char* part2 = "\",\"params\": [{\"name\": \"ipAddress\", \"value\": \"";
    const char* part3 = "\"}]}]}";

    snprintf(buf, size-1, "%s%s%s%s%s", part1, device_id, part2, local_ip_address, part3);
    return buf;
}
/*
 * {"measures": [{"deviceId": "Aiox-11038", "params": [{"name": "cloud", "value": "https://app.alter-presencepro.com"}]}]}
 */
const char* pr_make_main_url_change_notification4cloud(char* buf, size_t size, const char* main_url, const char* device_id) {
    const char* part1 = "{\"measures\": [{\"deviceId\": \"";
    const char* part2 = "\",\"params\": [{\"name\": \"cloud\", \"value\": \"";
    const char* part3 = "\"}]}]}";

    snprintf(buf, size-1, "%s%s%s%s%s", part1, device_id, part2, main_url, part3);
    return buf;
}
/*
 * {"measures": [{"deviceId": "<Proxy device id>", "params": [{"name": "reboot", "value": "<1-before reboot 2 - after reboot>"}]}]}
 */
const char* pr_make_reboot_alert4cloud(char* buf, size_t size, const char* device_id, pr_reboot_param_t status) {
    const char* part1 = "{\"measures\": [{\"deviceId\": \"";   
    const char* part2 = "\", \"params\": [{\"name\": \"reboot\", \"value\": \"";
    const char* part3 = "\"}]}]}";

    snprintf(buf, size-1, "%s%s%s%d%s", part1, device_id, part2, status, part3);
    return buf;
}

/******************************************************************************************************
 * {"agent_request": "connect"}
 * Return 1 if the obj contains this message
 */
int pr_agent_connected(msg_obj_t* obj) {
    if(!obj) return 0;
    cJSON* field;
    if(field = cJSON_GetObjectItem(obj, AGENT_REQUEST), !field) return 0;
    if(!strcmp(AGENT_CONNECT, field->valuestring)) return 1;
    return 0;
}
/******************************************************************************************************
 * Return 1 if the cmd is from cloud. Based on cloud_commands[] array
 * @param cmd
 * @return 1 if from cloud 0 id not
 */
int pr_is_cloud_command(pr_cmd_t cmd) {
    int i = 0;
    while(cloud_commands[i] != PR_CMD_UNDEFINED) {
        if(cloud_commands[i] == cmd) return 1;
        i++;
    }
    return 0;
}
/*
    PR_ALERT_FWU_FAILED = 1, PR_ALERT_FWU_READY_4_INSTALL = 2, PR_ALERT_MONITOR = 3,
{"alerts":[
    {"alertId":	"1", "deviceId": "<ProxyId>", "alertType": "<pr_alert_t>", "timestamp":	time_t, "paramsMap": {"alertText": "<text message>"}}]
}
*/
static const char* make_alert4WUD(char* buf, size_t size, pr_alert_t status, const char* diagnostics, const char* comp, const char* device_id) {
    const char* part1 = "{\"alerts\":[{\"alertId\": \"12345\", \"deviceId\": \"";
    const char* part2 = "\", \"alertType\": \"";   
    const char* part3 = "\", \"timestamp\":";    
    const char* part4 = ", \"paramsMap\": {\"";  
    const char* part5 = "\": \"";   
    const char* part6 = "\"}}]}";

    if(diagnostics)
        snprintf(buf, size-1, "%s%s%s%d%s%lu%s%s%s%s%s", part1, device_id, part2, status, part3, time(NULL), part4, alertText, part5, diagnostics, part6);
    else
        snprintf(buf, size-1, "%s%s%s%d%s%lu%s%s%s%s%s", part1, device_id, part2, status, part3, time(NULL), part4, component, part5, comp, part6);
    return buf;
}

/*
    NB! special case!!!
    PR_ALERT_WATCHDOG = 4
{"wud_ping": [{"deviceId":"gateway device id", "paramsMap":{"component":"<component_name>"}}]}
*/
static pr_alert_item_t processWUDping(msg_obj_t* alert_item) {
    pr_alert_item_t ret;
    ret.alert_type = PR_ALERT_UNDEFINED;
    cJSON* wp = cJSON_GetObjectItem(alert_item, paramsMap);
    if(!wp) return ret;
    wp = cJSON_GetObjectItem(wp, component);
    if(!wp) return ret;
    ret.alert_wd.alert_type = PR_ALERT_WATCHDOG;
    strncpy(ret.alert_wd.component, wp->valuestring, sizeof(ret.alert_wd.component)-1);
    ret.alert_wd.component[sizeof(ret.alert_wd.component)-1] = '\0';
    return ret;
}

const char* pr_make_fw_fail4WUD(char* buf, size_t size, const char* device_id) {
    return make_alert4WUD(buf, size, PR_ALERT_FWU_FAILED, "Firmware upgrade fails. Pitty.", NULL, device_id);
}

const char* pr_make_fw_ok4WUD(char* buf, size_t size, const char* device_id) {
    return make_alert4WUD(buf, size, PR_ALERT_FWU_READY_4_INSTALL, "Firmware upgrade ready for installation. Do you beleive it?!", NULL, device_id);
}

/* {"wud_ping": [{"deviceId":"<gateway device id>", "paramsMap":{"component":"<component_name>"}}] */
const char* pr_make_wd_alert4WUD(char* buf, size_t size, const char* comp, const char* device_id) {
    const char* part1 = "{\"wud_ping\": [{\"deviceId\":\""; 
    const char* part2 = "\", \"paramsMap\":{\"component\":\""; 
    const char* part3 = "\"}}]}";

    snprintf(buf, size-1, "%s%s%s%s%s", part1, device_id, part2, comp, part3);
    return buf;
}
/*
 * Send file to cloud - internal Agent-WUD command. IPCam Agent uses it!
 * buf         - buffer to store the message
 * size        - buffer size
 * files_type   - 'A' - audio, 'V' - video, 'S' - cound, 'P' - photo
 * files_list   - JSON array of files with full path: "filesList":["name1",..."nameN"]
 *
 * Return pointer to the buf as
 * {"name": "sendFiles", "type": <fileTypeString", "filesList": ["<filename>", ..., "<filename>"]}
*/
const char* pr_make_send_files4WUD(char* buf, size_t size, char files_type, const char* files_list) {
    snprintf(buf, size-1, "{\"name\": \"sendFiles\", \"type\": \"%c\", %s}", files_type, files_list);
    buf[size-1] = '\0';
    return buf;
}

/* {"commands": [
        {"deviceId":"aioxGW-0000B0C8AD000654", "type":0, parameters": [
		    {"name": cloudConnection","value":connected"},
            {"name":"deviceAuthToken","value":r0oHxX/OAx0QpgS/Rei9Hh4H7PpFO45NobBL0iz8bs="},
			{"name":"connString","value":"https://sboxall.presencepro.com"}
        ]}
    ]}
*/
const char* pr_conn_state_notf_to_agent(char* buf, size_t size, const char* device_id, int connect, const char* auth_token, const char* conn_string) {
    const char *conn_msg = "{\"commands\": ["
                           "{\"deviceId\":\"%s, \"type\":0, parameters\": ["    /* device_id */
                           "{\"name\": cloudConnection\",\"value\":%s\"},"      /* connected/disconnected */
                           "{\"name\":\"deviceAuthToken\",\"value\"%s\"},"      /* auth_token */
                           "{\"name\":\"connString\",\"value\":\"%s\"}"         /* conn_string */
                           "]}"
                           "]}";
    const char* conn_yes = "connected";
    const char* conn_no = "disconnected";

    snprintf(buf, size-1, conn_msg, device_id, (connect)?conn_yes:conn_no, auth_token, conn_string);

    buf[size-1] = '\0';
    return buf;
}

pr_alert_item_t pr_get_alert_item(msg_obj_t* alert_item) {
    pr_alert_item_t ret;

    /*WUD ping procesing as a special case */
    ret = processWUDping(alert_item);
    if(ret.alert_type != PR_ALERT_UNDEFINED) return ret;

    cJSON* a_type = cJSON_GetObjectItem(alert_item, alertType);
    if(!a_type) return ret;
    cJSON* parMap = cJSON_GetObjectItem(alert_item, paramsMap);
    if(!parMap) return ret;
    cJSON* diagnostics = cJSON_GetObjectItem(parMap, alertText);
    if(!diagnostics) return ret;

    switch(atoi(a_type->valuestring)) {
        case PR_ALERT_FWU_FAILED:
        case PR_ALERT_FWU_READY_4_INSTALL:
            ret.alert_type = (pr_alert_t)atoi(a_type->valuestring);
            break;
        case PR_ALERT_MONITOR:
            ret.alert_monitor.alert_type = PR_ALERT_MONITOR;
            strncpy(ret.alert_monitor.component, "All", sizeof(ret.alert_monitor.component)-1);
            ret.alert_monitor.component[sizeof(ret.alert_monitor.component)-1] = '\0';
            strncpy(ret.alert_monitor.reason, diagnostics->valuestring, sizeof(ret.alert_monitor.reason)-1);
            ret.alert_monitor.reason[sizeof(ret.alert_monitor.reason)-1] = '\0';
            break;
        default:
            ret.alert_type = PR_ALERT_UNDEFINED;
            break;
    }
    return ret;
}
/* {"commands":[{ "type": <number>, "commandId": "<number>", "deviceId": "<device_id>", "parameters": [{"name": "reboot", "value": "1"}]}]} */
const char* pr_make_reboot_command(char* buf, size_t size, const char* device_id) {
    snprintf(buf, size-1, "{\"commands\":[{ \"type\": 1, \"commandId\": \"0\", \"deviceId\": \"%s\", \"parameters\": [{\"name\": \"reboot\", \"value\": \"1\"}]}]}", device_id);
    return buf;
}

/*
Local funcs implementation
*/
/*
PR_CMD_FWU_START "parameters": [
{"name": "firmwareUpdateStatus","value": "1"},{"name": "firmwareUrl","value": "<url>"},{"name": "firmwareCheckSum","value": "<check_sum>"}]
PR_CMD_FWU_CANCEL "parameters": [{"name": "firmwareUpdateStatus","value": "0"}]
PR_CMD_CLOUD_CONN "parameters": [{"name":"connString", "value": "<url>"}, {"name":"deviceId", value": "<device_id>"}, {"name":"authToken", value": "<authToken>"}]
PR_CMD_CLOUD_OFF "parameters": [{"name": "cloudOff", "value": "1"}]
PG_CMD_REBOOT     "parameters": [{"name": "reboot", "value": "1"}]
PR_CMD_UPDATE_MAIN_URL: "parameters": [{"name": "cloud", "value": "url"}]
PR_CMD_SEND_FILES: "parameters": "[{"name
 */
static pr_cmd_item_t get_cmd_params_from_array(cJSON* params_array) {
    pr_cmd_item_t ret;
    ret.command_type = PR_CMD_UNDEFINED;
    unsigned int i;
    for(i = 0; i < cJSON_GetArraySize(params_array); i++) {
        cJSON* item = cJSON_GetArrayItem(params_array, i);

        char *name, *value;
        if(!get_name_value_pair(item, &name, &value)) {
            ret.command_type = PR_CMD_UNDEFINED;
            return ret;
        }
        if(!strcmp(name, cmd_fw_update_status) && !strcmp(value, "0")) {
            ret.fwu_start.command_type = PR_CMD_FWU_CANCEL;
            return ret;
        }
        if(!strcmp(name, cmd_fw_update_status) && !strcmp(value, "1")) {
            ret.fwu_start.command_type = PR_CMD_FWU_START;
        }
        else if(!strcmp(name, cmd_file_server_url)) {
            strncpy(ret.fwu_start.file_server_url, value, sizeof(ret.fwu_start.file_server_url)-1);
            ret.fwu_start.file_server_url[sizeof(ret.fwu_start.file_server_url)-1] = '\0';
        }
        else if(!strcmp(name, cmd_fw_update_check_sum)) {
            strncpy(ret.fwu_start.check_sum, value, sizeof(ret.fwu_start.check_sum)-1);
            ret.fwu_start.check_sum[sizeof(ret.fwu_start.check_sum)-1] = '\0';
        }
        else if(!strcmp(name, cmd_conn_string)) {
            ret.cloud_conn.command_type = PR_CMD_CLOUD_CONN;
            strncpy(ret.cloud_conn.conn_string, value, sizeof(ret.cloud_conn.conn_string)-1);
            ret.cloud_conn.conn_string[sizeof(ret.cloud_conn.conn_string)-1] = '\0';
        }
        else if(!strcmp(name, cmd_device_id)) {
            strncpy(ret.cloud_conn.device_id, value, sizeof(ret.cloud_conn.device_id)-1);
            ret.cloud_conn.device_id[sizeof(ret.cloud_conn.device_id)-1] = '\0';
        }
        else if(!strcmp(name, cmd_auth_token)) {
            strncpy(ret.cloud_conn.auth_token, value, sizeof(ret.cloud_conn.auth_token)-1);
            ret.cloud_conn.auth_token[sizeof(ret.cloud_conn.auth_token)-1] = '\0';
        }
        else if(!strcmp(name, cmd_firmware)) {
            strncpy(ret.cloud_conn.fw_version, value, sizeof(ret.cloud_conn.fw_version)-1);
            ret.cloud_conn.fw_version[sizeof(ret.cloud_conn.fw_version)-1] = '\0';
        }
        else if(!strcmp(name, cmd_reboot) && !strcmp(value, "1")) {
            ret.command_type = PR_CMD_REBOOT;
        }
        else if(!strcmp(name, cmd_main_url_cloud)) {
            ret.update_main_url.command_type = PR_CMD_UPDATE_MAIN_URL;
            strcpy(ret.update_main_url.main_url, value);
        }
        else if(!strcmp(name, cmd_cloud_off)) {
            ret.command_type = PR_CMD_CLOUD_OFF;
        }
        else {
            ret.command_type = PR_CMD_UNDEFINED;
        }
    }
    return ret;
}

/*
PR_CMD_RESTART_CHILD "paramsMap": {"restartChild": "<child_name>"}
PR_CMD_CLOUD_CONN "paramsMap": {"connString": "<url>", "deviceId": "<device_id>", "authToken": "<authToken>"}
PR_CMD_STOP "paramsMap": {"shutYouselfDown": "1"}
PR_CMD_UPDATE_MAIN_URL: "paramsMap": {"cloud": "url"}
 */
static pr_cmd_item_t get_cmd_params_from_map(cJSON* params_map) {
    pr_cmd_item_t ret;
    char* val = get_parameter(params_map, cmd_restart);
    if(val) {
        ret.restart_child.command_type = PR_CMD_RESTART_CHILD;
        strcpy(ret.restart_child.component, val);
    }
    else if(val = get_parameter(params_map, cmd_stop), val) {
        ret.command_type = PR_CMD_STOP;
    }
    else if(val = get_parameter(params_map, cmd_main_url_cloud), val) {
        ret.update_main_url.command_type = PR_CMD_UPDATE_MAIN_URL;
        strcpy(ret.update_main_url.main_url, val);
    }
    else {
        ret.command_type = PR_CMD_UNDEFINED;
    }
    return ret;
}

static int cmp_device_id(cJSON* item, const char* device_id) {
    cJSON* d_id = cJSON_GetObjectItem(item, deviceId);
    if(!d_id) return 0;
    return (strcmp(d_id->valuestring, device_id) == 0);
}

static cJSON* add_ref2array(cJSON* array, cJSON* ref) {
    if(!(array)) array = cJSON_CreateArray();
    cJSON_AddItemReferenceToArray(array, ref);
    return array;
}

static int get_name_value_pair(cJSON* cmd_item, char** name, char** value) {
    char* val = get_parameter(cmd_item, cmd_par_name);
    if(!val) {
        pu_log(LL_ERROR, "%s item not found in the command parameters array", cmd_par_name);
        return 0;
    }
    else
        *name = val;
    val = get_parameter(cmd_item, cmd_par_value);
    if(!val) {
        pu_log(LL_ERROR, "%s item not found in the command parameters array", cmd_par_value);
        return 0;
    }
    else
        *value = val;
    return 1;
}

static char* get_parameter(cJSON* cmd_item, const char* name) {
    cJSON* par = cJSON_GetObjectItem(cmd_item, name);
    if(!par) return NULL;
    return par->valuestring;
}