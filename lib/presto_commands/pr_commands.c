//
// Created by gsg on 08/12/16.
//

#include <string.h>
#include <malloc.h>
#include "pr_commands.h"

//fw_pgrade thread alerts
#define FWU_INIT_FAILED_MSG     "Firmware upgrade initiation failed"
#define FWU_DOWNLOAD_FAIL_MSG   "Firmware file(s) download failed"
#define FWU_CHECK_FAIL_MSG      "Firmware check file(s) failed"
#define FWU_MOVE_FAIL_MSG       "Firmware file(s) move to 'Upgrade' folder fails"
#define WFU_INIT_OK             "Firmware upgrade process start"
#define FWU_DOWNLOAD_OK_MSG     "Firmware file(s) download completed"
#define FWU_CHECK_OK_MSG        "Firmware check file(s) completed"
#define FWU_MOVE_OK_MSG         "Firmware files are ready for upgrade"
#define FWU_DOWNLOAD_BUSY_ALERT "Download directory was busy"
#define FWU_UPGRADE_BUSY_ALERT  "Upgrade directory was busy"
#define FWU_CANCELL_ALERT       "Firmware upgrade process cancel"

static const char* FWU_MESSAGES[PR_FWU_SIZE] = {
        FWU_INIT_FAILED_MSG, FWU_DOWNLOAD_FAIL_MSG, FWU_CHECK_FAIL_MSG, FWU_MOVE_FAIL_MSG, WFU_INIT_OK,
        FWU_DOWNLOAD_OK_MSG, FWU_CHECK_OK_MSG, FWU_MOVE_OK_MSG, FWU_DOWNLOAD_BUSY_ALERT, FWU_UPGRADE_BUSY_ALERT,
        FWU_CANCELL_ALERT
 };
//
//WUD alerts
#define WUD_START "WUD start"
#define WUD_CHILD_RESTART "WUD component restart"
#define WUD_REBOOT "WUD is going to reboot"

static const char* WUD_MESSAGES[PR_WUD_SIZE] = {WUD_START, WUD_CHILD_RESTART, WUD_REBOOT};
static char* PROC_NAMES[WA_SIZE] = {0};
///////////////////////////////////////////////////////////
//some helpers
static const char* find_str_value(cJSON* obj, const char* field); //returns NULL if fot found
static int find_int_value(cJSON* obj, const char* field); //returns -1 if not found
static void pr_json_2_command(cJSON* obj, pr_message_t *msg, const char* json_string);
static int create_err_msg(cJSON* obj, char* err_text );
static int create_monitor_alert(cJSON* obj, pr_mon_alert_body_t monitor_alert);
static int create_watchdog_alert(cJSON* obj, char* watchdog_alert);
static int create_fwu_alert(cJSON* obj, pr_fwu_alert_body_t fw_upgrade_alert);
static int create_wud_alert(cJSON* obj, pr_wud_alert_body_t wud_alert);
static int create_command(cJSON* obj, pr_command_t command);

const char* pr_chld_2_string(wa_child_t child_name) {
    if(!wa_child_t_range_check(child_name)) return NULL;

    return PROC_NAMES[child_name];
}
wa_child_t pr_string_2_chld(const char* child_name) {
    for(unsigned int i = 0; i < WA_SIZE; i++) {
        if(!strcmp(child_name, PROC_NAMES[i])) return (wa_child_t)i;
    }
    return WA_SIZE;
}
void pr_store_child_name(int child_name, const char* name) {
    if(!wa_child_t_range_check(child_name) || !name) return;
    if(PROC_NAMES[child_name]) free(PROC_NAMES[child_name]);
    PROC_NAMES[child_name] = strdup(name);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//common items:
static const char* proxyId = "proxyId";
static const char* messageType = "messageType";
//
//Error message
/*
{
"proxyId": "<GATEWAY_ID>",
"messageType": <messageType>                      -- PR_MSG_ERROR 
"error_text": "<error_text>" 
}
*/
//item names for error message
//
static const char* error_text = "error_text";
//see static int create_err_msg
//
//Monitor alert JSON template
/*
{
"proxyId": "<GATEWAY_ID>",
"messageType": <messageType>                      -- PR_MONITOR_ALERT
"alerts": [
{
"alertId": <alertId>,                   --unique ID
"alertType": "<alertType>",             -- currently just 0 - will be added later
"aletrTimestamp" :"<AlertTimestamp>"    --time_t
"paramsMap": {
"component": "<component>",         -- process name
"errorCode": "<errorCode>"          -- diagnostics
}
}
]
}
*/
//item names for monitor alert
static const char* mn_alerts = "alerts";
static const char* mn_alertId = "alertId";
static const char* mn_alertType = "alertType";
static const char* mn_alertTimestamp = "aletrTimestamp";
static const char* mn_paramsMap = "paramsMap";
static const char* mn_component = "component";
static const char* mn_errorCode = "errorCode";
//see static int create_monitor_alert()

//Watchdog alert structure
/*
{
"messageType": <messageType> --PR_WATCHDOG_ALERT
"component": "<component>",             -- process name
}
*/
///////////////////////////////////////////////////////////////////////////////////////////////////
//item names for watchdog alert
static const char* wd_component = "component";
//see static int create_watchdog_alert()
//
//Firmware upgrade alert structure
/*
{
"deficeID": "<deviceID>"
"messageType": <messageType>            --pr_msg_t = PR_FWU_ALERT
 "alertType": <alertType>               -- pr_fwu_msg_t
"timestamp": <timestamp>                -- time_t
"error": "<error_message>",             -- diagnostics
}
*/
///////////////////////////////////////////////////////////////////////////////////////////////////
//item names forfirmware upgrade alerts
static const char* fw_timestamp = "timestamp";
static const char* fw_alertType = "alertType";
static const char* fw_error = "error";
//
// see static int create_fwu_alert
//WUD alert structure
/*
{
"deficeID": "<deviceID>"
""messageType": <messageType>            --pr_msg_t = PR_WUD_ALERT
"timestamp": <timestamp>                -- time_t
"process_name": "<process_name>"        --
"error": "<error_message>",             -- diagnostics
}
*/ 
///////////////////////////////////////////////////////////////////////////////////////////////////
//item names for WUD alert
static const char* wud_timestamp = "timestamp";
static const char* wud_process_name = "process_name";
static const char* wud_error = "error";
//
//see static int create_wud_alert()
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Command message structure (WUD internal)
/*
{

"messageType": <messageType>            --PR_COMMAND
"commandID": <commandID> --PR_FW_UPGRADE_START = 1, PR_FW_UPGRADE_CANCEL = 2, PR_STOP, PR_RESTART_CHILD = 3, PR_PRESTO_INFO = 4,  PR_WATCHDOG = 5,
"commandBody": {
<command_body>                      -- depeding on command type
}
<command_body>:
PR_FW_UPGRADE_START:
"conn_string":  "<conn_string>"
PR_FW_UPGRADE_CANCEL:
        PR_STOP:
<empty>
        PR_RESTART_CHILD:
"child_index":  <wa_child_t>        --Child process #. Currently 0 or 1
PR_PRESTO_INFO:
"cloud_conn_string":    "<cloud_connection_string>"
"device_id":            "<device_id>"
"activation_token":     "<activation_token>"
PR_WATCHDOG:
"process_name":         "<process_name_from_config>"
*/
///////////////////////////////////////////////////////////////////////////////////////////////////
//item names for cmd
static const char* cmd_commandID = "commandID";
static const char* cmd_commandBody = "commandBody";
static const char* cmd_conn_string = "conn_string";
static const char* cmd_child_index = "child_index";
static const char* cmd_cloud_conn_string = "cloud_conn_string";
static const char* cmd_device_id = "device_id";
static const char* cmd_activation_token = "activation_token";
//
//see static int create_command
//
///////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////
pr_message_t pr_json_2_msg(const char* json_string) {
    pr_message_t ret;
    const char* err_msg = "";
    const char* str_value= "";
    int int_value;

    cJSON* obj = cJSON_Parse(json_string);
    if(!obj) {
        err_msg = "Error in JSON message ";
        goto on_error;
    }
    if(int_value = find_int_value(obj, messageType), !pr_msg_type_t_range_check(int_value)) {
        err_msg = "\"messageType\" item not found or not an int type or has bad value in json string ";
        goto on_error;
    }
    ret.message_type = (pr_msg_type_t)int_value;
    if(str_value = find_str_value(obj, proxyId), str_value == NULL) {
        err_msg = "\"proxyId\" item not found or not a string type in json string ";
        goto on_error;
    }
    ret.deviceID = strdup(str_value);
    switch(ret.message_type) {
        case PR_MSG_ERROR:
            if(str_value = find_str_value(obj, error_text), str_value == NULL) {
                err_msg = "\"error_text\" item not found or not a string type in json string ";
                goto on_error;
            }
            ret.err_text = strdup(str_value);
            break;
        case PR_MONITOR_ALERT: {
            cJSON* al;
            cJSON* pm;
            if (al = cJSON_GetObjectItem(obj, mn_alerts), al == NULL) {
                err_msg = "\"alerts\" item not found in json string ";
                goto on_error;
            }
            if(int_value = find_int_value(al, mn_alertId), int_value < 0) {
                err_msg = "\"alertId\" item not found or not an int type or has bad value in json string ";
                goto on_error;
            }
            ret.monitor_alert.alertId = (unsigned int)int_value;
            if(int_value = find_int_value(al, mn_alertType), !pr_mon_alert_t_range_check(int_value)) {
                err_msg = "\"alertType\" item not found or not an int type or has bad value in json string ";
                goto on_error;
            }
            ret.monitor_alert.alertType = (pr_mon_alert_t)int_value;
            if(int_value = find_int_value(al, mn_alertTimestamp), int_value < 0) {
                err_msg = "\"alertTimestamp\" item not found or not an int type or has bad value in json string ";
                goto on_error;
            }
            ret.monitor_alert.alertTimestamp = int_value;
            if (pm = cJSON_GetObjectItem(obj, mn_paramsMap), pm == NULL) {
                err_msg = "\"paramsMap\" item not found in json string ";
                goto on_error;
            }
            if(str_value = find_str_value(pm, mn_component), str_value == NULL) {
                err_msg = "\"component\" item not found or not a string type in json string ";
                goto on_error;
            }
            ret.monitor_alert.proc_name = strdup(str_value);
            if(str_value = find_str_value(pm, mn_errorCode), str_value == NULL) {
                err_msg = "\"errorCode\" item not found or not a string type in json string ";
                goto on_error;
            }
            ret.monitor_alert.errorCode = strdup(str_value);
        }
            break;
        case PR__WATCHDOG_ALERT:
            if(str_value = find_str_value(obj, wd_component), str_value == NULL) {
                err_msg = "\"component\" item not found or not a string type in json string ";
                goto on_error;
            }
            ret.watchdog_alert = strdup(str_value);
             break;
        case PR_FWU_ALERT:
            if(int_value = find_int_value(obj, fw_timestamp), int_value < 0) {
                err_msg = "\"timestamp\" item not found or not an int type or has bad value in json string ";
                goto on_error;
            }
            ret.fw_upgrade_alert.timestamp = int_value;
            if(int_value = find_int_value(obj, fw_alertType), !pr_fwu_msg_t_range_check(int_value)) {
                err_msg = "\"alertType\" item not found or not an int type or has bad value in json string ";
                goto on_error;
            }
            ret.fw_upgrade_alert.alert_type = (pr_fwu_msg_t)int_value;
            if(str_value = find_str_value(obj, fw_error), str_value == NULL) {
                err_msg = "\"error\" item not found or not a string type in json string ";
                goto on_error;
            }
            ret.fw_upgrade_alert.error = strdup(str_value);
            break;
        case PR_WUD_ALERT:
            if(int_value = find_int_value(obj, wud_timestamp), int_value < 0) {
                err_msg = "\"timestamp\" item not found or not an int type or has bad value in json string ";
                goto on_error;
            }
            ret.wud_alert.timestamp = int_value;
            if(str_value = find_str_value(obj, wud_process_name), str_value == NULL) {
                err_msg = "\"process_name\" item not found or not a string type in json string ";
                goto on_error;
            }
            ret.wud_alert.proc_name = strdup(str_value);
            if(str_value = find_str_value(obj, wud_error), str_value == NULL) {
                err_msg = "\"error\" item not found or not a string type in json string ";
                goto on_error;
            }
            ret.wud_alert.error = strdup(str_value);
            break;
        case PR_COMMAND:
            pr_json_2_command(obj, &ret, json_string);  //it makes an error msg by itself if needed
            break;
        default:
            err_msg = "pr_json_2_msg: Unsupported command ID!";
            goto on_error;
            break;
    }
    return ret;
    on_error: {
        char *msg = malloc(strlen(err_msg) + strlen(json_string) + 1);
        sprintf(msg, "%s%s", err_msg, json_string);
        ret.message_type = PR_MSG_ERROR;
        ret.err_text = msg;
        return ret;
    }
}
void pr_erase_msg(pr_message_t msg) {
    if (msg.deviceID) free(msg.deviceID);
    switch (msg.message_type) {
        case PR_MSG_ERROR:
            if (msg.err_text) free(msg.err_text);
            break;
        case PR_MONITOR_ALERT:
            if (msg.monitor_alert.proc_name) free(msg.monitor_alert.proc_name);
            if (msg.monitor_alert.errorCode) free(msg.monitor_alert.errorCode);
            break;
        case PR__WATCHDOG_ALERT:
            if (msg.watchdog_alert) free(msg.watchdog_alert);
            break;
        case PR_FWU_ALERT:
            if (msg.fw_upgrade_alert.error) free(msg.fw_upgrade_alert.error);
            break;
        case PR_WUD_ALERT:
            if (msg.wud_alert.proc_name) free(msg.wud_alert.proc_name);                        // process name
            if (msg.wud_alert.error) free(msg.wud_alert.error);
            break;
        case PR_COMMAND:
            switch (msg.command.cmd) {
                 case PR_FW_UPGRADE_START:
                    if(msg.command.fw_conn_str)free(msg.command.fw_conn_str);
                    break;
                case PR_PRESTO_INFO:
                    if(msg.command.presto_info.cloud_conn_string)free(msg.command.presto_info.cloud_conn_string);
                    if(msg.command.presto_info.device_id)free(msg.command.presto_info.device_id);
                    if(msg.command.presto_info.activation_token)free(msg.command.presto_info.activation_token);
                    break;
                default:
                    break;
            }
            break;
        default:
            break;
    }
}
int pr_make_message(char** buf, pr_message_t msg) {
    cJSON* obj;
    obj = cJSON_CreateObject();
    cJSON_AddItemToObject(obj, proxyId, cJSON_CreateString(msg.deviceID));
    cJSON_AddItemToObject(obj, messageType, cJSON_CreateNumber(msg.message_type));
    switch(msg.message_type) {
        case PR_MSG_ERROR:
            create_err_msg(obj, msg.err_text);
            break;
        case PR_MONITOR_ALERT:
            create_monitor_alert(obj, msg.monitor_alert);
            break;
        case PR__WATCHDOG_ALERT:
            create_watchdog_alert(obj, msg.watchdog_alert);
            break;
        case PR_FWU_ALERT:
            create_fwu_alert(obj, msg.fw_upgrade_alert);
            break;
        case PR_WUD_ALERT:
            create_wud_alert(obj, msg.wud_alert);
            break;
        case PR_COMMAND:
            create_command(obj, msg.command);
            break;
        default:
            break;
    }
    *buf = strdup(cJSON_Print(obj));

    cJSON_Delete(obj);
    return 1;
}
////////////////////////////////////////////////////////////////////////
int wa_child_t_range_check(int item) {
    return (item > 0) && (item < WA_SIZE);
}
int pr_command_id_t_range_check(int item) {
    return (item > 0) && (item < PR_CMD_SIZE);
}
int pr_msg_type_t_range_check(int item) {
    return (item > 0) && (item < PR_MSG_SIZE);
}
int pr_fwu_msg_t_range_check(int item) {
    return (item > 0) && (item < PR_FWU_SIZE);
}
int pr_wud_msg_t_range_check(int item) {
    return (item > 0) && (item < PR_WUD_SIZE);
}
int pr_mon_alert_t_range_check(int item) {
    return (item > 0) && (item < PR_MON_ALERT_SIZE);
}
//
int pr_make_wud_alert(char** buf, pr_wud_msg_t wud_msg_type, const char* deviceID, const char* proc_name) {
    pr_message_t msg;
    msg.message_type = PR_WUD_ALERT;
    msg.deviceID = strdup(deviceID);
    msg.wud_alert.timestamp = time(NULL);
    msg.wud_alert.error = (pr_wud_msg_t_range_check(wud_msg_type))?strdup(WUD_MESSAGES[wud_msg_type]):strdup("Message type range error");
    msg.wud_alert.proc_name = strdup(proc_name);

    pr_make_message(buf, msg);
    pr_erase_msg(msg);
    return 1;
}
int pr_make_fwu_alert(char** buf, pr_fwu_msg_t fwu_alert, const char* deviceID, const char* proc_name) {
    pr_message_t msg;
    msg.message_type = PR_FWU_ALERT;
    msg.fw_upgrade_alert.alert_type = fwu_alert;
    msg.fw_upgrade_alert.error = (pr_fwu_msg_t_range_check(fwu_alert))?strdup(FWU_MESSAGES[fwu_alert]):strdup("Message type range error");
    msg.fw_upgrade_alert.timestamp = time(NULL);

    pr_make_message(buf, msg);
    pr_erase_msg(msg);
    return 1;
}
///////////////////////////////////////////////////////////////////////////////
//returns NULL in fot found
static const char* find_str_value(cJSON* obj, const char* field) {
    cJSON *it = cJSON_GetObjectItem(obj, field);
    if (it == NULL) return NULL;
    if(it->type != cJSON_String) return NULL;
    return it->string;
}
//return -1 if smth wrong
static int find_int_value(cJSON* obj, const char* field) { //return -1 if smth wrong
    cJSON *it = cJSON_GetObjectItem(obj, field);
    if (it == NULL) return -1;
    if(it->type != cJSON_Number) return -1;
    return it->valueint;
}
static void pr_json_2_command(cJSON* obj, pr_message_t *msg, const char* json_string) {
    const char* err_msg;
    const char* str_value;
    int int_value;

    if(int_value = find_int_value(obj, cmd_commandID), !pr_command_id_t_range_check(int_value)) {
        err_msg = "\"commandID\" item not found or not an int type or has bad value in json string ";
        goto on_error;
    }
    msg->command.cmd = (pr_command_id_t)int_value;
    cJSON* cb;
    if(cb = cJSON_GetObjectItem(obj, cmd_commandBody), cb == NULL) {
        err_msg = "\"commandBody\" item not found in json string ";
        goto on_error;
    }
    switch(msg->command.cmd) {
        case PR_FW_UPGRADE_START:
            if(str_value = find_str_value(obj, cmd_conn_string), str_value == NULL) {
                err_msg = "\"conn_string\" item not found or not a string type in json string ";
                goto on_error;
            }
            msg->command.fw_conn_str = strdup(str_value);
            break;
        case PR_FW_UPGRADE_CANCEL:
            break;
        case PR_STOP:
            break;
        case PR_RESTART_CHILD:
            if(int_value = find_int_value(obj, cmd_child_index), !wa_child_t_range_check(int_value)) {
                err_msg = "\"child_index\" item not found or not an int type or has bad value in json string ";
                goto on_error;
            }
            msg->command.restart_chld = (wa_child_t)int_value;
            break;
        case PR_PRESTO_INFO:
            if(str_value = find_str_value(obj, cmd_cloud_conn_string), str_value == NULL) {
                err_msg = "\"cloud_conn_string\" item not found or not a string type in json string ";
                goto on_error;
            }
            msg->command.presto_info.cloud_conn_string = strdup(str_value);

            if(str_value = find_str_value(obj, cmd_device_id), str_value == NULL) {
                err_msg = "\"device_id\" item not found or not a string type in json string ";
                goto on_error;
            }
            msg->command.presto_info.device_id = strdup(str_value);

            if(str_value = find_str_value(obj, cmd_activation_token), str_value == NULL) {
                err_msg = "\"activation_token\" item not found or not a string type in json string ";
                goto on_error;
            }
            msg->command.presto_info.activation_token = strdup(str_value);
            break;
        default:
            err_msg = "pr_json_2_command: Unsupported command ID!";
            goto on_error;
    }
    return;
    on_error: {
        char *err = malloc(strlen(err_msg) + strlen(json_string) + 1);
        sprintf(err, "%s%s", err_msg, json_string);
        msg->message_type = PR_MSG_ERROR;
        msg->err_text = err;
        return;
    }
}
static int create_err_msg(cJSON* obj, char* err_text ) {
    cJSON_AddItemToObject(obj, error_text, cJSON_CreateString(err_text));
    return 1;
}
static int create_monitor_alert(cJSON* obj, pr_mon_alert_body_t monitor_alert) {
    cJSON* array = cJSON_CreateArray();
    cJSON* arr_item = cJSON_CreateObject();
    cJSON_AddItemToObject(arr_item, mn_alertId, cJSON_CreateNumber(monitor_alert.alertId));
    cJSON_AddItemToObject(arr_item, mn_alertType, cJSON_CreateNumber(monitor_alert.alertType));
    cJSON_AddItemToObject(arr_item, mn_alertTimestamp, cJSON_CreateNumber(monitor_alert.alertTimestamp));
    cJSON* params_map = cJSON_CreateObject();
    cJSON_AddItemToObject(params_map, mn_component, cJSON_CreateString(monitor_alert.proc_name));
    cJSON_AddItemToObject(params_map, mn_errorCode, cJSON_CreateString(monitor_alert.errorCode));
//Attaching pieces
    cJSON_AddItemToObject(arr_item, mn_paramsMap, params_map);
    cJSON_AddItemToArray(array, arr_item);
    cJSON_AddItemToObject(obj, mn_alerts, array);
    return 1;
}
static int create_watchdog_alert(cJSON* obj, char* watchdog_alert) {
    cJSON_AddItemToObject(obj, wd_component, cJSON_CreateString(watchdog_alert));
    return 1;
}
static int create_fwu_alert(cJSON* obj, pr_fwu_alert_body_t fw_upgrade_alert) {
    cJSON_AddItemToObject(obj, fw_timestamp, cJSON_CreateNumber(fw_upgrade_alert.timestamp));
    cJSON_AddItemToObject(obj, fw_alertType, cJSON_CreateNumber(fw_upgrade_alert.alert_type));
    cJSON_AddItemToObject(obj, fw_error, cJSON_CreateString(fw_upgrade_alert.error));
    return 1;
}
static int create_wud_alert(cJSON* obj, pr_wud_alert_body_t wud_alert) {
    cJSON_AddItemToObject(obj, wud_timestamp, cJSON_CreateNumber(wud_alert.timestamp));
    cJSON_AddItemToObject(obj, wud_process_name, cJSON_CreateString(wud_alert.proc_name));
    cJSON_AddItemToObject(obj, wud_error, cJSON_CreateString(wud_alert.error));
    return 1;
}
static int create_command(cJSON* obj, pr_command_t command) {
    cJSON_AddItemToObject(obj, cmd_commandID, cJSON_CreateNumber(command.cmd));
    cJSON* body = cJSON_CreateObject();
    switch(command.cmd) {
        case PR_FW_UPGRADE_START:
            cJSON_AddItemToObject(body, cmd_conn_string, cJSON_CreateString(command.fw_conn_str));
            break;
         case PR_RESTART_CHILD:
            cJSON_AddItemToObject(body, cmd_child_index, cJSON_CreateNumber(command.restart_chld));
            break;
        case PR_PRESTO_INFO:
            cJSON_AddItemToObject(body, cmd_cloud_conn_string, cJSON_CreateString(command.presto_info.cloud_conn_string));
            cJSON_AddItemToObject(body, cmd_device_id, cJSON_CreateString(command.presto_info.device_id));
            cJSON_AddItemToObject(body, cmd_activation_token, cJSON_CreateString(command.presto_info.activation_token));
            break;
        default:
            return 0;
            break;
    }
    cJSON_AddItemToObject(obj, cmd_commandBody, body);
    return 1;
}
