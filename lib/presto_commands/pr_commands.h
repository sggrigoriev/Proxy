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
 

 Created by gsg on 08/12/16.

 Contains Presto commands & alerts internal representation + conversion to/from JSON utilities
  has to be refactored once apon a time
*/
#ifndef PRESTO_PR_COMMANDS_H
#define PRESTO_PR_COMMANDS_H

#include <unistd.h>
#include <time.h>
#include <lib_http.h>

#include "cJSON.h"

#define PR_ALERT_MSG_SIZE           120
#define PR_MAX_PROC_NAME_SIZE       17      /*TASK_SCHED_LEN+1 */

/*Child processes - currently Proxy & Agent */
typedef enum {PR_PROC1, PR_PROC_2, PR_CHILD_SIZE
} pr_child_t;

/*Save child process name. The process name used for processes recognition and management
  child_name  - the value mapped on pr_child_t enums
  name        - process name to be stored
NB!If the child name does not match the enum - assertion called
*/
void pr_store_child_name(int child_name, const char* name);

/*Get the process name by process index enum
  child_name  - process index
Return the process name
*/
const char* pr_chld_2_string(pr_child_t child_name);

/*Get the process index by process name
  child_name  - process name
Return process index or PR_CHILD_SIZE
*/
pr_child_t pr_string_2_chld(const char* child_name);

/*Process index range check
  item    - process index to be checked
Return 1 if OK, retirn 0 id item < 0 or > max index
*/
int pr_child_t_range_check(int item);
/*/
Messaging part

Message typr definition - just to hide the JSON from all other places of use
*/
typedef cJSON msg_obj_t;

/*Message types:
PR_COMMANDS_MSG   - commands
PR_ALERTS_MSG     - alerts
PR_OTHER_MSG      - watchdogs + some strange messages to Agent
*/
typedef enum {PR_COMMANDS_MSG, PR_ALERTS_MSG, PR_OTHER_MSG, PR_MSG_TYPE_SIZE
} pr_msg_type_t;

/* Proxy-WUD commands+Cloud-Proxy commands (PR_COMMANDS_MSG subtypes)
PR_CMD_UNDEFINED        - for unrecognized commands
PR_CMD_FWU_START        - start firmware upgrade    (cloud -> Proxy)
PR_CMD_FWU_CANCEL       - cancel firmware upgrade   (cloud -> Proxy)
PR_CMD_RESTART_CHILD    - restart one child         (Watchdog->WUD)
PR_CMD_CLOUD_CONN       - cloud connections parameters (Proxy -> WUD; Proxy IO threads tp Proxy main)
PR_CMD_CLOUD_OFF        - cloud connection off signal (Proxy IO threads to Proxy main)
PR_CMD_STOP             - ??? just for some case
PR_CMD_UPDATE_MAIN_URL  - update main cloud url     (cloud -> Proxy)
PR_CMD_REBOOT           - command for reboot        (watchdog->WUD)
*/
typedef enum {PR_CMD_UNDEFINED, PR_CMD_FWU_START, PR_CMD_FWU_CANCEL, PR_CMD_RESTART_CHILD, PR_CMD_CLOUD_CONN, PR_CMD_CLOUD_OFF,
    PR_CMD_STOP,
    PR_CMD_UPDATE_MAIN_URL, PR_CMD_REBOOT,
    PR_CMD_SIZE
} pr_cmd_t;
static const pr_cmd_t cloud_commands[] = {  /* Here is the list of commands sent by cloud. This is needed to separate internal ones from cloud */
        PR_CMD_FWU_START,
        PR_CMD_FWU_CANCEL,
        PR_CMD_UPDATE_MAIN_URL,
        PR_CMD_UNDEFINED
};
/* Proxy-Cloud "reboot" parameter values
 * PR_NO_REBOOT         - send on recurrig basis with fw version
 * PR_BEFORE_REBOOT     - send right before the reboot requested
 * PR_AFTER_REBOOT      - send on startup step
*/
typedef enum {PR_BEFORE_REBOOT=1, PR_AFTER_REBOOT=2
} pr_reboot_param_t;

/*Body for PR_CMD_FWU_START, PR_CMD_FWU_CANCEL NB! for cancel params except command_type are not valid */
typedef struct {
    pr_cmd_t command_type;          /*PR_CMD_FWU_START, PR_CMD_FWU_CANCEL NB! for cancel params below are not valid */
    char file_server_url[LIB_HTTP_MAX_URL_SIZE];    /*full path to the file */
    char check_sum[LIB_HTTP_SHA_256_SIZE+1];        /*SHA_256 size */
} pr_cmd_fwu_start_t;

/*Body for PR_CMD_RESTART_CHILD command */
typedef struct {
    pr_cmd_t command_type;
    char component[PR_MAX_PROC_NAME_SIZE];  /*process name to be restarted */
} pr_cmd_restart_t;

/*Body for PR_CMD_CLOUD_CONN */
typedef struct {
    pr_cmd_t command_type;      /*PR_CMD_CLOUD_CONN*/
    char conn_string[LIB_HTTP_MAX_URL_SIZE];                /*cloud connection URL*/
    char device_id[LIB_HTTP_DEVICE_ID_SIZE];                /*gateway device_id*/
    char auth_token[LIB_HTTP_AUTHENTICATION_STRING_SIZE];   /*gateway authentication token*/
    char fw_version[LIB_HTTP_FW_VERSION_SIZE];              /*gateway firmware version*/
} pr_cmd_cloud_t;

/*Body for PR_CMD_UPDATE_MAIN_URL */
typedef struct {
    pr_cmd_t command_type;          /*PR_CMD_UPDATE_MAIN_URL */
    char main_url[LIB_HTTP_MAX_URL_SIZE];
} pr_cmd_update_main_t;

/*Internal command presentation */
typedef union {
    pr_cmd_t                command_type; /*just head to understand which field from the union is valid */
    pr_cmd_fwu_start_t      fwu_start;
    pr_cmd_restart_t        restart_child;
    pr_cmd_cloud_t          cloud_conn;
    pr_cmd_update_main_t    update_main_url;
} pr_cmd_item_t;
/*
Common functions


Parse JSON string to JSON object. NB! The object shouild be deleted!
  msg     - JSON string
Returns JSON object or NULL if parsing error
*/
msg_obj_t* pr_parse_msg(char* msg);

/*Erase JSON object
  msg - JSON object to be deleted
*/
void pr_erase_msg(msg_obj_t* msg);

/*Gey the JSON array size
  array   - suppose to be array
Return the size of 0 if array is empty or the object is not an array
*/
size_t pr_get_array_size(msg_obj_t* array);

/*Get array's item by index
  array   - the array
  idx     - the index
Return the i-th object from array
*/
msg_obj_t* pr_get_arr_item(msg_obj_t* array, size_t idx);

/*Convert JSON obect to JSON string
  msg         - JSON object to be converted
  text_msg    - the buffer for the converted string
  size        - buffer size
*/
void pr_obj2char(msg_obj_t* obj_msg, char* text_msg, size_t size);

/*********************************************
 * Get cJSON object with commands array
 * @param obj - parsed commands message -
 * @return    - array or NULL if smth wrong
 */
msg_obj_t* pr_get_cmd_array(msg_obj_t* obj);

/**********************************************
 * Create the array object size 1 from one element
 * NB! New memory allocated for returned value!
 * @param cmd_arr_elem - JSON command element
 * @return - the array size 1
 */
msg_obj_t* pr_make_cmd_array(msg_obj_t* cmd_arr_elem);

/*********************************************
 * Get cJSON object with alerts array.
 * NB! wud_ping processed as a separate case
 * @param obj - parsed alerts message
 * @return    - arrat or NULL if smth wrong
 */
msg_obj_t* pr_get_alerts_array(msg_obj_t* obj);

/*Command type recognition by command item - parameters mostly. The command defines by its parameters... New experience for me. But very modern one
  cmd_item    - element from commands array.
Return commant type
*/
pr_cmd_item_t pr_get_cmd_item(msg_obj_t* cmd_item);

/*This function splits the commands array into Proxy-related and Agent-related sub-arrays. For now these commands sets are not intersects which is great
  msg             - original message as JSON object
  device_id       - gateway device id - one of ways how to understand the command belongs to Proxy, not ot the end-device
  msg4proxy       - buffer for commands to Proxy converted to the JSON string. Could be "" if no commands for Proxy
  msg4proxy_size  - buffer size
  msg4agent       - buffer for commands to Agent converted to the JSON string. Could be "" if no commands for Agent
  msg4agent_size  - buffer size
*/
void pr_split_msg(msg_obj_t* msg, const char* device_id, char* msg4proxy, size_t msg4proxy_size, char* msg4agent, size_t msg4agent_size);

/*Get the message type: command, alert or other
  msg - message as JSON object
Return message type.
*/
pr_msg_type_t pr_get_message_type(msg_obj_t* msg);

/*
Commands generation

Create the connection info command (PR_CMD_CLOUD_CONN)
  buf             - buffer to save the command
  size            - buffer size
  conn_string     - cloud connection URL
  device_id       - gateway device id
  auth_token      - gateway authentication token
  version         - gateway firmware version
Return pointer to the buf
*/
const char* pr_make_conn_info_cmd(char* buf, size_t size, const char* conn_string, const char* device_id, const char* auth_token, const char* version);

/*********************************************************************
 * Make cloud off command. Used to pass the off-line status from Proxy SERVER_READ & SERVER_WRITE functions to the Proxy_main
 * Symmetrical command (about the on-line status -> PR_CMD_CLOUD_CONN (pr_make_conn_info_cmd)
 * @param buf       - buffer to put the command
 * @param size      - buffer size
 * @param device_id - Proxy ID
 * @return      - pointer to the buffer
 */
const char* pr_make_cloud_off_cmd(char* buf, size_t size, const char* device_id);

/*Create child process restart command (PR_CMD_RESTART_CHILD)
  buf             - buffer to save the command
  size            - buffer size
  child_name      - process name
Return pointer to the buf
*/
const char* pr_make_restart_child_cmd(char* buf, size_t size, const char* child_name);
/*
Alerts part


Firmware upgrade statuses. NB! They;re inline with cloud constants - do no change w/o sync with cloud team
*/
typedef enum {PR_FWU_STATUS_STOP = 0, PR_FWU_STATUS_START = 1, PR_FWU_STATUS_PROCESS = 2, PR_FWU_STATUS_FAIL = 3
} fwu_status_t;

/*Alert types
PR_ALERT_UNDEFINED            - for unrecognized alerts
PR_ALERT_FWU_FAILED           - firmware upgrade failure alert
PR_ALERT_FWU_READY_4_INSTALL  - firmware checked and ready for install (means reboot for gateway)
PR_ALERT_MONITOR              - not used until the WUD monitor is not ready
PR_ALERT_WATCHDOG             - watchdog alert
*/
typedef enum {PR_ALERT_UNDEFINED, PR_ALERT_FWU_FAILED , PR_ALERT_FWU_READY_4_INSTALL, PR_ALERT_MONITOR, PR_ALERT_WATCHDOG
} pr_alert_t;

/*Body for PR_MONITOR_ALERT */
typedef struct {
    pr_alert_t alert_type;
    time_t timestamp;
    char component[PR_MAX_PROC_NAME_SIZE];      /*process name*/
    char reason[PR_ALERT_MSG_SIZE];             /*diagnostics*/
} pr_alert_monitor_t;

/*Body for PR_ALERT_WATCHDOG*/
typedef struct {
    pr_alert_t alert_type;
    char component[PR_MAX_PROC_NAME_SIZE];  /*Alert sender name*/
} pr_watchdog_alert_t;

/*Alert message body*/
typedef union {
    pr_alert_t          alert_type;
    pr_alert_monitor_t  alert_monitor;
    pr_watchdog_alert_t alert_wd;
} pr_alert_item_t;

/*Create message to inform cloud about fw upgrade status 
  buf         - buffer to store the message
  size        - buffer size
  status      - firmware upgrade status
  fw_version  - gateway current firmware version
  device_id   - gateway device id
  Return pointer to buf
*/
const char* pr_make_fw_status4cloud(char* buf, size_t size, fwu_status_t status, const char* fw_version, const char* device_id);

/***************************************************************************************************
 * Create messge to infom the Cloud about local GW IP address. Sendind after each reconnect
 * @param buf               -
 * @param size
 * @param local_ip_address
 * @param device_id
 * @return
 */
const char* pr_make_local_ip_notification(char* buf, size_t size, const char* local_ip_address, const char* device_id);

/****************************************************************************************************
 * Creates the alert for cloud about the main URL change:
 * {"measures": [{"deviceId": "Aiox-11038",
 * "params": [{"name": "cloud", "value": "https://app.alter-presencepro.com"}]}]}
 *
 * @param buf           - buffer for alert
 * @param size          - buffer size
 * @param device_id     - the proxy device id
 * @return - the buffer address
 */
const char* pr_make_main_url_change_notification4cloud(char* buf, size_t size, const char* main_url, const char* device_id);

/*Create reboot alert to be sent to the cloud
  buf         - buffer to store the message
  size        - buffer size
  m_alert     - alert body
  device_id   - gateway device id
  val         - reboot status (see pr_reboot_param_t)
Return pointer to buf
*/
const char* pr_make_reboot_alert4cloud(char* buf, size_t size, const char* device_id, pr_reboot_param_t status);

/*Wrapper for pr_make_fw_status4cloud status FAIL */
const char* pr_make_fw_fail4WUD(char* buf, size_t size, const char* device_id);

/*Wrapper for pr_make_fw_status4cloud status READY FOR INSTALL */
const char* pr_make_fw_ok4WUD(char* buf, size_t size, const char* device_id);

/*Create watchdog alert
  buf         - buffer to store the message
  size        - buffer size
  component   - process name which sends it
  device_id   - gateway device_id
Return pointer to the buf
*/
const char* pr_make_wd_alert4WUD(char* buf, size_t size, const char* component, const char* device_id);

/*************************************************************************************************
 * Create the notification to the Agent about no/off line cloud connection status
 * @param buf       - buffer to store the message
 * @param size      - buffer size
 * @param device_id - Proxy device ID
 * @param connect   - 0 Proxy of line, 1 - Proxy online
 * @param auth_token- Proxy authentication token
 * @param conn_string- Proxy Main URL
 * @return          - the buffer with the command
 */
const char* pr_conn_state_notf_to_agent(char* buf, size_t size, const char* device_id, int connect, const char* auth_token, const char* conn_string);

/*Recognise the alert type by alert item
  alert_item  - alert item as JSON object
Return alert type
*/
pr_alert_item_t pr_get_alert_item(msg_obj_t* alert_item);

/******************************************************************************************************
 * {"agent_request": "connect"}
 * Return 1 if the obj contains this message
 */
int pr_agent_connected(msg_obj_t* obj);

/******************************************************************************************************
 * Return 1 if the cmd is from cloud. Based on cloud_commands[] array
 * @param cmd
 * @return 1 if from cloud 0 id not
 */
int pr_is_cloud_command(pr_cmd_t cmd);

/******************************************************************************************************
 * Create the command string: "{ "type": <number>, "commandId": "<number>", "deviceId": "<device_id>", "parameters": [{"name": "reboot", "value": "1"}]}"
 * @param buf           buffer for command string made
 * @param size          buffer size
 * @param device_id     Proxy deviceID
 * @return              Command made
 */
const char* pr_make_reboot_command(char* buf, size_t size, const char* device_id);

#endif /*PRESTO_PR_COMMANDS_H */