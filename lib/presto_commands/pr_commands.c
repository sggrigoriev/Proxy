//
// Created by gsg on 08/12/16.
//

#include <string.h>
#include <malloc.h>
#include <assert.h>
#include <pthread.h>

#include "pr_commands.h"

static const char ALERT_TYPES[PR_ALERT_SIZE][PR_ALERT_TYPE_PREFFIX_LEN] = {
"ERR", "MON", "WDT", "WFU", "URL", "CMD"
};
static const char ALERT_DELIM = '-';
///////////////////////////////////////////////////////////////////////////////////////////////////
// the strucure: {"alerts":[{"alertId":<seq#>,"deviceId":"<proxyID>","alertType":"CCC-NNN","timestamp":<NNNNNNN>,"paramsMap"{...}}]}
//header items:
static const char* alerts = "alerts";
static const char* alertId = "alertId";
static const char* deviceId = "deviceId";
//common items:
static const char* alertType = "alertType";
static const char* timestamp = "timestamp";
static const char* paramsMap = "paramsMap";
//Error alert
static const char* error_text = "errorText";
//Monitor alert:
static const char* component = "component";
static const char* reason = "reason";
//Upgrade alert, update alert:
static const char* alertText = "alertText";
///////////////////////////////////////////////////////////////////////////////////////////////////
//item names for cmd
static const char* cmd_file_server_url = "fileServerURL";
static const char* cmd_conn_string = "connString";
static const char* cmd_device_id = "deviceId";
static const char* cmd_auth_token = "authToken";
//

static pthread_mutex_t  own_mutex;

static char PROC_NAMES[PR_CHILD_SIZE][PR_MAX_PROC_NAME_SIZE] = {0};
///////////////////////////////////////////////////////////
//some helpers
static int find_str_value(cJSON* obj, const char* field, char* val, size_t size); //returns 1 of OK
static int find_int_value(cJSON* obj, const char* field, int* val); //returns 1 if OK

static cJSON* skip_alert_head(cJSON* obj);
static int get_head(cJSON* obj, pr_alert_head_t* header, char** err_text);
static int make_struct_head(pr_alert_t* alert, pr_alert_head_t header, char** err_text);

//Takes the string parameter from paramsMap
//return 1 if OK
static int get_str_par(cJSON* obj, const char* field, char* value, size_t size, char** err_text);
static const char* create_alert_type(int prefix, char delim, int postfix, char* buf, size_t size);
//ts ignored if it is 0
static cJSON* add_alert_head(cJSON* obj, const char* deviceID, unsigned int seqNo);
static int create_alert_head(cJSON* obj, int prefix, int postfix, time_t ts);
static cJSON* create_par_map(cJSON* obj);
static int add_str_2_par_map(cJSON* pm, const char* field, const char* value);

//////////////////////////////////////////////////////////////////////////////////////
void pr_store_child_name(int child_name, const char* name) {
    assert(pr_child_t_range_check(child_name));
    assert(name);
    pthread_mutex_lock(&own_mutex);
        strncpy(PROC_NAMES[child_name], name, PR_MAX_PROC_NAME_SIZE-1);
    pthread_mutex_unlock(&own_mutex);
}
const char* pr_chld_2_string(pr_child_t child_name) {
    assert(pr_child_t_range_check(child_name));
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

///////////////////////////////////////////////////////////////////////////////////////////
//All are thread-protected except range checks
//////////////////////////////////////////////////////////////////////////////////////////
//return 1 if the json doesn't contain alerts
int pr_is_command(const char* json) {
    return (strstr(json, alerts) == NULL);
}

void pr_json_2_struct(pr_alert_t* alert, const char* json_string) {
    char* err_text;
    pr_alert_head_t header;

    pthread_mutex_lock(&own_mutex);

    cJSON* obj = cJSON_Parse(json_string);
    cJSON* cobj;
    if(!obj) {
        err_text = "Error in JSON message ";
        goto on_error;
    }
    cobj = (pr_is_command(json_string))?obj:skip_alert_head(obj);
    if(!cobj) {
        err_text = "Error in alerts structure ";
        goto on_error;
    }
    if(!get_head(cobj, &header, &err_text)) goto on_error;
    if(!make_struct_head(alert, header, &err_text)) goto on_error; //creating fixed part of alert structure
    switch(alert->alert_type) {     //creating variable (paramsMap) part of alert structure
        case PR_ERROR_ALERT:
            if(!get_str_par(cobj, error_text, alert->error.error_text, sizeof(alert->error.error_text), &err_text)) goto on_error;
            break;
        case PR_MONITOR_ALERT:
            if(!get_str_par(cobj, component, alert->monitor.component, sizeof(alert->monitor.component), &err_text)) goto on_error;
            if(!get_str_par(cobj, reason, alert->monitor.reason, sizeof(alert->monitor.reason), &err_text)) goto on_error;
            break;
        case PR_WATCHDOG_ALERT:
            if(!get_str_par(cobj, component, alert->watchdog.component, sizeof(alert->watchdog.component), &err_text)) goto on_error;
            break;
        case PR_FWU_ALERT:
            if(!get_str_par(cobj, alertText, alert->fwu.alert_text, sizeof(alert->fwu.alert_text), &err_text)) goto on_error;
            break;
        case PR_URL_ALERT:
            if(!get_str_par(cobj, alertText, alert->url.alert_text, sizeof(alert->url.alert_text), &err_text)) goto on_error;
            break;
        case PR_COMMAND:
            switch(header.alert_subtype) {
                case PR_CMD_FWU_START:
                    if(!get_str_par(cobj, cmd_file_server_url, alert->cmd_fwu.file_server_url, sizeof(alert->cmd_fwu.file_server_url), &err_text)) goto on_error;
                    break;
                case PR_CMD_FWU_CANCEL:
                case PR_CMD_STOP:
                    break;
                case PR_CMD_RESTART_CHILD:
                    if(!get_str_par(cobj, component, alert->cmd_restart.component, sizeof(alert->cmd_restart.component), &err_text)) goto on_error;
                    break;
                case PR_CMD_CLOUD_CONN:
                    if(!get_str_par(cobj, cmd_conn_string, alert->cmd_cloud.conn_string, sizeof(alert->cmd_cloud.conn_string), &err_text)) goto on_error;
                    if(!get_str_par(cobj, cmd_device_id, alert->cmd_cloud.device_id, sizeof(alert->cmd_cloud.device_id), &err_text)) goto on_error;
                    if(!get_str_par(cobj, cmd_auth_token, alert->cmd_cloud.auth_token, sizeof(alert->cmd_cloud.auth_token), &err_text)) goto on_error;
                    break;
                default:
                    err_text = "Unsupported alert subtype";
                    goto on_error;
            }
            break;
        default:
            err_text = "Unsupported alert type";
            goto on_error;
    }
    cJSON_Delete(obj);
    pthread_mutex_unlock(&own_mutex);
    return;
    on_error: {
        alert->error.alert_type = PR_ERROR_ALERT;
        alert->error.error_alert_type = PR_ERROR_TYPE;
        alert->error.tmestamp = time(NULL);
        strncpy(alert->error.error_text, err_text, sizeof(alert->error.error_text));
    if(obj) cJSON_Delete(obj);
    pthread_mutex_unlock(&own_mutex);
    }
    return;
}
int pr_struct_2_json(char* buf, size_t size, pr_alert_t msg, const char* deviceID, unsigned int seqNo) {
    cJSON* obj;
    cJSON* cobj;
    pthread_mutex_lock(&own_mutex);
    obj = cJSON_CreateObject();
    cobj = (msg.alert_type == PR_COMMAND)?obj:add_alert_head(obj, deviceID, seqNo);
    switch(msg.alert_type) {
        case PR_ERROR_ALERT:
            create_alert_head(cobj, msg.error.alert_type, msg.error.error_alert_type, msg.error.tmestamp);
            add_str_2_par_map(create_par_map(cobj), error_text, msg.error.error_text);
            break;
        case PR_MONITOR_ALERT: {
            create_alert_head(cobj, msg.monitor.alert_type, msg.monitor.monitor_alert_type, msg.monitor.timestamp);
            cJSON *p = create_par_map(cobj);
            add_str_2_par_map(p, component, msg.monitor.component);
            add_str_2_par_map(p, reason, msg.monitor.reason);
            break;
        }
        case PR_WATCHDOG_ALERT:
            create_alert_head(cobj, msg.watchdog.alert_type, msg.watchdog.watchdog_alert_type, msg.watchdog.timestamp);
            add_str_2_par_map(create_par_map(cobj), component, msg.watchdog.component);
            break;
        case PR_FWU_ALERT:
            create_alert_head(cobj, msg.fwu.alert_type, msg.fwu.fwu_alert_type, msg.fwu.timestamp);
            add_str_2_par_map(create_par_map(cobj), alertText, msg.fwu.alert_text);
            break;
        case PR_URL_ALERT:
            create_alert_head(cobj, msg.url.alert_type, msg.url.url_alert_type, msg.url.timestamp);
            add_str_2_par_map(create_par_map(obj), alertText, msg.url.alert_text);
            break;
        case PR_COMMAND:
            switch(msg.cmd_type) {
                case PR_CMD_FWU_START:
                    create_alert_head(cobj, msg.cmd_fwu.alert_type, msg.cmd_fwu.command_type, 0);
                    add_str_2_par_map(create_par_map(cobj), cmd_file_server_url, msg.cmd_fwu.file_server_url);
                    break;
                case PR_CMD_FWU_CANCEL:
                    create_alert_head(cobj, msg.cmd_fwu.alert_type, msg.cmd_fwu.command_type, 0);
                    break;
                case PR_CMD_RESTART_CHILD:
                    create_alert_head(cobj, msg.cmd_restart.alert_type, msg.cmd_restart.command_type, 0);
                    add_str_2_par_map(create_par_map(cobj), component, msg.cmd_restart.component);
                    break;
                case PR_CMD_CLOUD_CONN: {
                    create_alert_head(cobj, msg.cmd_cloud.alert_type, msg.cmd_cloud.command_type, 0);
                    cJSON *p = create_par_map(cobj);
                    add_str_2_par_map(p, cmd_conn_string, msg.cmd_cloud.conn_string);
                    add_str_2_par_map(p, cmd_device_id, msg.cmd_cloud.device_id);
                    add_str_2_par_map(p, cmd_auth_token, msg.cmd_cloud.auth_token);
                    break;
                }
                case PR_CMD_STOP:
                    create_alert_head(cobj, msg.cmd_stop.alert_type,msg.cmd_stop.command_type, 0);
                    break;
                default:
                    break;
            }
            break;
        default:
            break;
    }
    strncpy(buf, cJSON_PrintUnformatted(obj), size-1);

    cJSON_Delete(obj);
    pthread_mutex_unlock(&own_mutex);
    return 1;
}
////////////////////////////////////////////////////////////////////////
//Enum range chackers
///////////////////////
int pr_child_t_range_check(int item) {
    return (item >= 0) && (item < PR_CHILD_SIZE);
}
int pr_alert_type_range_check(int item) {
    return (item >= 0) && (item < PR_ALERT_SIZE);
}
int pr_error_alert_t_range_check(int item) {
    return (item >= 0) && (item < PR_ERROR_SIZE);
}
int pr_monitor_alert_t_range_check(int item) {
    return (item >= 0) && (item < PR_MONITOR_SIZE);
}
int pr_watchdog_alert_t_range_check(int item) {
    return (item >= 0) && (item < PR_WATCHDOG_SIZE);
}
int pr_fwu_alert_t_range_check(int item) {
    return (item >= 0) && (item < PR_FWU_SIZE);
}
int pr_url_alert_t_range_check(int item) {
    return (item >= 0) && (item < PR_URL_SIZE);
}
int pr_cmd_fwu_t_range_check(int item) {
    return (item == PR_CMD_FWU_START)||(item == PR_CMD_FWU_CANCEL);
}
int pr_cmd_restart_t_range_check(int item) {
    return item == PR_CMD_RESTART_CHILD;
}
int pr_cmd_cloud_t_range_check(int item) {
    return item == PR_CMD_CLOUD_CONN;
}
int pr_cmd_stop_range_check(int item) {
    return (item == PR_CMD_STOP);
}
//
//Make the JSON alert from valuable data
int pr_make_error_alert(const char* error_text, const char* deviceID, unsigned int seqNum, char* json, size_t size) {
    pr_alert_t a;
    a.error.alert_type = PR_ERROR_ALERT;
    a.error.error_alert_type = PR_ERROR_TYPE;
    a.error.tmestamp = time(NULL);
    strncpy(a.error.error_text, error_text, sizeof(a.error.error_text));

    return pr_struct_2_json(json, size, a, deviceID, seqNum);
}
int pr_make_monitor_alert(pr_monitor_alert_t mat, const char* comp, const char* rsn, const char* deviceID, unsigned int seqNum, char* json, size_t size) {
    pr_alert_t a;
    a.monitor.alert_type = PR_MONITOR_ALERT;
    a.monitor.monitor_alert_type = mat;
    a.monitor.timestamp = time(NULL);
    strncpy(a.monitor.component, comp, sizeof(a.monitor.component));
    strncpy(a.monitor.reason, rsn, sizeof(a.monitor.reason));
    return pr_struct_2_json(json, size, a, deviceID, seqNum);
}
int pr_make_wd_alert(const char* component, const char* deviceID, unsigned int seqNum, char* json, size_t size) {
    pr_alert_t a;
    a.watchdog.alert_type = PR_WATCHDOG_ALERT;
    a.watchdog.watchdog_alert_type = PR_WATCHDOG_TYPE;
    a.watchdog.timestamp = time(NULL);
    strncpy(a.watchdog.component, component, sizeof(a.watchdog.component));
    return pr_struct_2_json(json, size, a, deviceID, seqNum);
}
int pr_make_fwu_alert(pr_fwu_alert_t fwua_type, const char* alert_txt, const char* deviceID, unsigned int seqNum, char* json, size_t size) {
    pr_alert_t a;
    a.fwu.alert_type = PR_FWU_ALERT;
    a.fwu.fwu_alert_type = fwua_type;
    a.fwu.timestamp = time(NULL);
    strncpy(a.fwu.alert_text, alert_txt, sizeof(a.fwu.alert_text));
    return pr_struct_2_json(json, size, a, deviceID, seqNum);
}
int pr_make_url_alert(pr_url_alert_t urla_type, const char* alert_txt, const char* deviceID, unsigned int seqNum, char* json, size_t size) {
    pr_alert_t a;
    a.url.alert_type = PR_URL_ALERT;
    a.url.url_alert_type = urla_type;
    a.url.timestamp = time(NULL);
    strncpy(a.url.alert_text, alert_txt, sizeof(a.url.alert_text));
    return pr_struct_2_json(json, size, a, deviceID, seqNum);
}
int pr_make_cmd_fwu(pr_cmd_t cmd_type, const char* fs_url, char* json, size_t size) {
    pr_alert_t a;
    a.cmd_fwu.alert_type = PR_COMMAND;
    a.cmd_fwu.command_type = cmd_type;
    if(cmd_type == PR_CMD_FWU_START) strncpy(a.cmd_fwu.file_server_url, fs_url, sizeof(a.cmd_fwu.file_server_url));
    return pr_struct_2_json(json, size, a, NULL, 0);
}
int pr_make_cmd_restart(const char* component, char* json, size_t size) {
    pr_alert_t a;
    a.cmd_restart.alert_type = PR_COMMAND;
    a.cmd_restart.command_type = PR_CMD_RESTART_CHILD;
    strncpy(a.cmd_restart.component, component, sizeof(a.cmd_restart.component));
    return pr_struct_2_json(json, size, a, NULL, 0);
}
int pr_make_cmd_cloud(const char* cs, const char* di, const char* at, char* json, size_t size) {
    pr_alert_t a;
    a.cmd_cloud.alert_type = PR_COMMAND;
    a.cmd_cloud.command_type = PR_CMD_CLOUD_CONN;
    strncpy(a.cmd_cloud.conn_string, cs, sizeof(a.cmd_cloud.conn_string));
    strncpy(a.cmd_cloud.device_id, di, sizeof(a.cmd_cloud.device_id));
    strncpy(a.cmd_cloud.auth_token, at, sizeof(a.cmd_cloud.auth_token));
    return pr_struct_2_json(json, size, a, NULL, 0);
}
int pr_make_cmd_stop(char* json, size_t size) {
    pr_alert_t a;
    a.cmd_stop.alert_type = PR_COMMAND;
    a.cmd_stop.command_type = PR_CMD_STOP;
    return pr_struct_2_json(json, size, a, NULL, 0);
}

///////////////////////////////////////////////////////////////////////////////
//returns 1 if OK
static int find_str_value(cJSON* obj, const char* field, char* val, size_t size) {
    cJSON *it = cJSON_GetObjectItem(obj, field);
    if (it == NULL) return 0;
    if(it->type != cJSON_String) return 0;
    strncpy(val, it->valuestring, size-1);
    return 1;
}
//return -1 if smth wrong
static int find_int_value(cJSON* obj, const char* field, int* val) { //return 1 if OK
    cJSON *it = cJSON_GetObjectItem(obj, field);
    if (it == NULL) return 0;
    if(it->type != cJSON_Number) return 0;
    *val = it->valueint;
    return 1;
}

static cJSON* skip_alert_head(cJSON* obj) {
    cJSON* ret = cJSON_GetObjectItem(obj, alerts);
    if(!ret) return NULL;
    if(cJSON_GetArraySize(ret) < 1) return NULL;    //empty alerts array
//NB! Currently I'm ready to have just one item in array. Fuck the rest!
    return cJSON_GetArrayItem(ret, 0);
}
static int get_head(cJSON* obj, pr_alert_head_t* header, char** err_text) {
    char alert_type[PR_MAX_ALERT_TYPE_SIZE];

    if(!find_str_value(obj, alertType, alert_type, sizeof(alert_type))) {
        *err_text = "\"alertType\" item not found";
        return 0;
    }
    alert_type[PR_ALERT_TYPE_PREFFIX_LEN] = '\0'; // instead of delimeter
    header->alert_type = -1;
    unsigned int i;
    for(i = 0; i < PR_ALERT_SIZE; i++) {
        if(!strcmp(alert_type, ALERT_TYPES[i])) {
            header->alert_type = i;
            break;
        }
    }
    char* endptr;
    header->alert_subtype = (int)strtol(alert_type+PR_ALERT_TYPE_PREFFIX_LEN+PR_ALERT_TYPE_DELIM_LEN, &endptr, 10);
    if(header->alert_type == PR_COMMAND) return 1;  //command hasn't timestamp
//Else get timestamp
    if(!find_int_value(obj, timestamp, (int*)(&header->timestamp))) {
        *err_text = "\"timestamp\" item not found";
        return 0;
    }
    return 1;
}
static int make_struct_head(pr_alert_t* alert, pr_alert_head_t header, char** err_text) {
    if(!pr_alert_type_range_check(header.alert_type)) {
        *err_text = "alert type value is out of range";
        return 0;
    }
    switch(header.alert_type) {
        case PR_ERROR_ALERT:
            if(!pr_error_alert_t_range_check(header.alert_subtype)) {
                *err_text = "error type number out of range";
                return 0;
            }
            alert->error.alert_type = PR_ERROR_ALERT;
            alert->error.error_alert_type = (pr_error_alert_t)header.alert_subtype;
            alert->error.tmestamp = header.timestamp;
            break;
        case PR_MONITOR_ALERT:
            if(!pr_monitor_alert_t_range_check(header.alert_subtype)) {
                *err_text = "monitor alert type number out of range";
                return 0;
            }
            alert->monitor.alert_type = PR_MONITOR_ALERT;
            alert->monitor.monitor_alert_type = (pr_monitor_alert_t)header.alert_subtype;
            alert->monitor.timestamp = header.timestamp;
            break;
        case PR_WATCHDOG_ALERT:
            if(!pr_watchdog_alert_t_range_check(header.alert_subtype)) {
                *err_text = "watchdog alert type number out of range";
                return 0;
            }
            alert->watchdog.alert_type = PR_WATCHDOG_ALERT;
            alert->watchdog.watchdog_alert_type = (pr_watchdog_alert_t)header.alert_subtype;
            alert->watchdog.timestamp = header.timestamp;
            break;
        case PR_FWU_ALERT:
            if(!pr_fwu_alert_t_range_check(header.alert_subtype)) {
                *err_text = "firmware upgrage alert type number out of range";
                return 0;
            }
            alert->fwu.alert_type = PR_FWU_ALERT;
            alert->fwu.fwu_alert_type = (pr_fwu_alert_t)header.alert_subtype;
            alert->fwu.timestamp = header.timestamp;
            break;
        case PR_URL_ALERT:
            if(!pr_url_alert_t_range_check(header.alert_subtype)) {
                *err_text = "URL update alert type number out of range";
                return 0;
            }
            alert->url.alert_type = PR_URL_ALERT;
            alert->url.url_alert_type = (pr_url_alert_t)header.alert_subtype;
            alert->url.timestamp = header.timestamp;
            break;
        case PR_COMMAND:
            switch(header.alert_subtype) {
                case PR_CMD_FWU_START:
                case PR_CMD_FWU_CANCEL:
                    alert->cmd_fwu.alert_type = PR_COMMAND;
                    alert->cmd_fwu.command_type = (pr_cmd_t)header.alert_subtype;
                    break;
                case PR_CMD_RESTART_CHILD:
                    alert->cmd_restart.alert_type = PR_COMMAND;
                    alert->cmd_restart.command_type = PR_CMD_RESTART_CHILD;
                    break;
                case PR_CMD_CLOUD_CONN:
                    alert->cmd_cloud.alert_type = PR_COMMAND;
                    alert->cmd_cloud.command_type = PR_CMD_CLOUD_CONN;
                    break;
                case PR_CMD_STOP:
                    alert->cmd_stop.alert_type = PR_COMMAND;
                    alert->cmd_stop.command_type = PR_CMD_STOP;

                default:
                    *err_text = "Unsupported command type";
                    return 0;
            }
            break;
        default:
            *err_text = "Unsupported alert type";
            return 0;
    }
    return 1;
}
static int get_str_par(cJSON* obj, const char* field, char* value, size_t size, char** err_text) {
    cJSON* p_map = cJSON_GetObjectItem(obj, paramsMap);
    if(!p_map) {
        *err_text = "\"paramsMap\" item not found";
        return 0;
    }
    if(!find_str_value(obj, field, value, size)) {
        *err_text = "item in \"paramsMap\" not found";
        return 0;
    }
    return 1;
}

static const char* create_alert_type(int prefix, char delim, int postfix, char* buf, size_t size) {
    strcpy(buf, ALERT_TYPES[prefix]);
    buf[PR_ALERT_TYPE_PREFFIX_LEN] = delim;
    sprintf(buf+PR_ALERT_TYPE_PREFFIX_LEN+PR_ALERT_TYPE_DELIM_LEN, "%d", postfix);
    return buf;
}

static cJSON* add_alert_head(cJSON* obj, const char* deviceID, unsigned int seqNo) {
    cJSON* arr = cJSON_CreateArray();
    cJSON_AddItemToObject(obj, alerts, arr);    //attach array to object
    cJSON* elem = cJSON_CreateObject();
    cJSON_AddItemToArray(arr, elem);            //add one element into aray
    cJSON_AddItemToObject(elem, alertId, cJSON_CreateNumber(seqNo));        //add alertId and deviceId to element
    cJSON_AddItemToObject(elem, deviceId, cJSON_CreateString(deviceID));
    return elem;    // return the array's element for further info adding
}
static int create_alert_head(cJSON* obj, int prefix, int postfix, time_t ts) {
    char buf[PR_MAX_ALERT_TYPE_SIZE];
    cJSON_AddItemToObject(obj, alertType, cJSON_CreateString(create_alert_type(prefix, ALERT_DELIM, postfix, buf, sizeof(buf))));
    if(ts) cJSON_AddItemToObject(obj, timestamp, cJSON_CreateNumber(ts));
    return 1;
}
static cJSON* create_par_map(cJSON* obj) {
    cJSON* p_map = cJSON_CreateObject();
    cJSON_AddItemToObject(obj, paramsMap, p_map);
    return p_map;
}
static int add_str_2_par_map(cJSON* pm, const char* field, const char* value) {
    cJSON_AddItemToObject(pm, field, cJSON_CreateString(value));
    return 1;
}

