//
// Created by gsg on 08/12/16.
//
// Contains Presto commands & alerts internal representation + conversion to/from JSON utilities
//
#ifndef PRESTO_PR_COMMANDS_H
#define PRESTO_PR_COMMANDS_H

#include <unistd.h>
#include <time.h>
#include <lib_http.h>

#include "cJSON.h"

#define PR_ALERT_MSG_SIZE           120
#define PR_MAX_PROC_NAME_SIZE       17      //TASK_SCHED_LEN+1

typedef enum {PR_PROC1, PR_PROC_2, PR_CHILD_SIZE
} pr_child_t;

void pr_store_child_name(int child_name, const char* name);
const char* pr_chld_2_string(pr_child_t child_name);
pr_child_t pr_string_2_chld(const char* child_name);
int pr_child_t_range_check(int item);
///////////////////////////////////////////////////////
//Messaging
typedef cJSON msg_obj_t;
typedef enum {PR_COMMANDS_MSG, PR_ALERTS_MSG, PR_OTHER_MSG, PR_MSG_TYPE_SIZE
} pr_msg_type_t;
// Proxy-WUD commands+Cloud-Proxy commands
typedef enum {PR_CMD_UNDEFINED, PR_CMD_FWU_START, PR_CMD_FWU_CANCEL, PR_CMD_RESTART_CHILD, PR_CMD_CLOUD_CONN, PR_CMD_STOP,
    PR_CMD_UPDATE_MAIN_URL,
    PR_CMD_SIZE
} pr_cmd_t;

typedef struct {
    pr_cmd_t command_type;          //PR_CMD_FWU_START, PR_CMD_FWU_CANCEL NB! for cancel params below are not valid
    char file_server_url[LIB_HTTP_MAX_URL_SIZE];    //full path to the file
    char file_name[LIB_HTTP_MAX_URL_SIZE];          //file name
    char check_sum[LIB_HTTP_SHA_256_SIZE+1];           //SHA_256 size
    char fw_version[LIB_HTTP_FW_VERSION_SIZE];
} pr_cmd_fwu_start_t;

typedef struct {
    pr_cmd_t command_type;      //PR_CMD_RESTART_CHILD
    char component[PR_MAX_PROC_NAME_SIZE];
} pr_cmd_restart_t;

typedef struct {
    pr_cmd_t command_type;      //PR_CMD_CLOUD_CONN
    char conn_string[LIB_HTTP_MAX_URL_SIZE];
    char device_id[LIB_HTTP_DEVICE_ID_SIZE];
    char auth_token[LIB_HTTP_AUTHENTICATION_STRING_SIZE];
} pr_cmd_cloud_t;

typedef struct {
    pr_cmd_t command_type;          //PR_CMD_UPDATE_MAIN_URL
    char main_url[LIB_HTTP_MAX_URL_SIZE];
} pr_cmd_update_main_t;

typedef union {
    pr_cmd_t               command_type; //just head to understand which field from the union is valid
    pr_cmd_fwu_start_t     fwu_start;
    pr_cmd_restart_t       restart_child;
    pr_cmd_cloud_t         cloud_conn;
    pr_cmd_update_main_t   update_main_url;
} pr_cmd_item_t;

//Returns NULL if bad
msg_obj_t* pr_parse_msg(const char* msg);
void pr_erase_msg(msg_obj_t* msg);

size_t pr_get_array_size(msg_obj_t* array);
msg_obj_t* pr_get_arr_item(msg_obj_t* array, size_t idx);

pr_cmd_item_t pr_get_cmd_item(msg_obj_t* cmd_item);

//Return obj_msg, converted to plain string
void pr_obj2char(msg_obj_t* obj_msg, char* text_msg, size_t size);
//Return commands array for Proxy or "", return full message w/o proxy commands for Agent or ""
void pr_split_msg(msg_obj_t* msg, const char* device_id, char* msg4proxy, size_t msg4proxy_size, char* msg4agent, size_t msg4agent_size);
//retrieva from msg_object message type
pr_msg_type_t pr_get_message_type(msg_obj_t* msg);
//same as previous bu for separate element
pr_msg_type_t pr_get_item_type(msg_obj_t* item);
////////////////////////
//Commands generation
const char* pr_make_conn_info_cmd(char* buf, size_t size, const char* conn_string, const char* device_id, const char* auth_token);
const char* pr_make_restart_child_cmd(char* buf, size_t size, const char* child_name);
////////////////////////////////////////////////////////
//Alerts
typedef enum {PR_FWU_STATUS_STOP = 0, PR_FWU_STATUS_START = 1, PR_FWU_STATUS_PROCESS = 2, PR_FWU_STATUS_FAIL = 3
} fwu_status_t;

typedef enum {PR_ALERT_UNDEFINED, PR_ALERT_FWU_FAILED , PR_ALERT_FWU_READY_4_INSTALL, PR_ALERT_MONITOR, PR_ALERT_WATCHDOG,
    PR_ALERT_SIZE
} pr_alert_t;
typedef struct {
    pr_alert_t alert_type;             //PR_MONITOR_ALERT, PR_ALERT_WATCHDOG
    time_t timestamp;
    char component[PR_MAX_PROC_NAME_SIZE];
    char reason[PR_ALERT_MSG_SIZE];
} pr_alert_monitor_t;
typedef struct {
    pr_alert_t alert_type;                 //PR_ALERT_WATCHDOG
    char component[PR_MAX_PROC_NAME_SIZE];
} pr_watchdog_alert_t;
typedef union {
    pr_alert_t          alert_type;
    pr_alert_monitor_t  alert_monitor;
    pr_watchdog_alert_t alert_wd;
} pr_alert_item_t;
//Create message to inform cloud about fw upgrade status
const char* pr_make_fw_status4cloud(char* buf, size_t size, fwu_status_t status, const char* fw_version, const char* device_id);
const char* pr_make_reboot_alert4cloud(char* buf, size_t size, const char* device_id);
const char* pr_make_fw_fail4WUD(char* buf, size_t size, const char* device_id);
const char* pr_make_fw_ok4WUD(char* buf, size_t size, const char* device_id);
const char* pr_make_monitor_alert4cloud(char* buf, size_t size, pr_alert_monitor_t m_alert, const char* device_id);
const char* pr_make_wd_alert4WUD(char* buf, size_t size, const char* component, const char* device_id);

pr_alert_item_t pr_get_alert_item(msg_obj_t* alert_item);

#endif //PRESTO_PR_COMMANDS_H