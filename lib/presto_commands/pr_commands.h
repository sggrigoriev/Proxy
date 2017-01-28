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
#define PR_ALERT_TYPE_PREFFIX_LEN   3
#define PR_ALERT_TYPE_DELIM_LEN     1
#define PR_ALERT_TYPE_NUMBER_LEN    3
#define PR_MAX_ALERT_TYPE_SIZE      PR_ALERT_TYPE_PREFFIX_LEN+PR_ALERT_TYPE_DELIM_LEN+PR_ALERT_TYPE_NUMBER_LEN+1

typedef enum {PR_PROC1, PR_PROC_2, PR_CHILD_SIZE
} pr_child_t;
///////////////////////////////////////////////////////
typedef enum {PR_ERROR_ALERT, PR_MONITOR_ALERT, PR_WATCHDOG_ALERT, PR_FWU_ALERT, PR_URL_ALERT, PR_COMMAND,
    PR_ALERT_SIZE
} pr_alert_type_t;

    typedef enum {PR_ERROR_TYPE, PR_ERROR_SIZE
    } pr_error_alert_t;         //Repeated from the LOG: hard errors insode the Proxy/WUD

    typedef enum {PR_MONITOR_RESTART, PR_MONITOR_REBOOT, PR_MONITOR_SIZE
    } pr_monitor_alert_t;       //WUD actions on monitored processes report

    typedef enum {PR_WATCHDOG_TYPE, PR_WATCHDOG_SIZE
    } pr_watchdog_alert_t;      //use same alert for wd from processes to WUD and from WUD to Cloud

    typedef enum {PR_FWU_INIT_FAILED, PR_FWU_DOWNLOAD_FAIL, PR_FWU_CHECK_FAIL, PR_FWU_MOVE_FAIL, PR_FWU_INIT_OK,
        PR_FWU_DOWNLOAD_OK, PR_FWU_CHECK_OK, PR_FWU_MOVE_OK, PR_FWU_DOWNLOAD_BUSY, PR_FWU_UPGRADE_BUSY,
        PR_FWU_CANCEL, PR_FWU_SIZE
    } pr_fwu_alert_t;           //FWU process feedback

    typedef enum {PR_URL_CP_OK, PR_URL_MAIN_OK, PR_URL_CP_ERR, PR_URL_MAIN_ERR, PR_URL_SIZE
    } pr_url_alert_t;           //URL reassignement feedback
/////////////////////////////////
// Proxy-WUD commands+Cloud-Proxy commands
    typedef enum {PR_CMD_FWU_START, PR_CMD_FWU_CANCEL, PR_CMD_RESTART_CHILD, PR_CMD_CLOUD_CONN, PR_CMD_STOP,
        PR_CMD_SIZE
    }pr_cmd_t;
///////////////////////////////////////////////////////
//
typedef struct {
    int alert_type;
    int alert_subtype;
    time_t timestamp;
} pr_alert_head_t;
//typedefs below reflect internal messages presentation
typedef struct {
    pr_alert_type_t alert_type;         //PR_ERROR_ALERT
    pr_error_alert_t error_alert_type;  //PR_ERROR_TYPE
    time_t tmestamp;
    char error_text[PR_ALERT_MSG_SIZE];
} pr_error_alert_body_t;

typedef struct {
    pr_alert_type_t alert_type;             //PR_MONITOR_ALERT
    pr_monitor_alert_t monitor_alert_type;
    time_t timestamp;
    char component[PR_MAX_PROC_NAME_SIZE];
    char reason[PR_ALERT_MSG_SIZE];
} pr_monitor_alert_body_t;

typedef struct {
    pr_alert_type_t alert_type;                 //PR_WATCHDOG_ALERT
    pr_watchdog_alert_t watchdog_alert_type;    //PR_WATCHDOG_TYPE
    time_t timestamp;
    char component[PR_MAX_PROC_NAME_SIZE];
} pr_watchdog_alert_body_t;

typedef struct {
    pr_alert_type_t alert_type;     //PR_FWU_ALERT
    pr_fwu_alert_t fwu_alert_type;
    time_t timestamp;
    char alert_text[PR_ALERT_MSG_SIZE];
} pr_fwu_alert_body_t;

typedef struct {
    pr_alert_type_t alert_type;     //PR_URL_ALERT
    pr_url_alert_t url_alert_type;
    time_t timestamp;
    char alert_text[PR_ALERT_MSG_SIZE];
} pr_url_alert_body_t;

typedef struct {
    pr_alert_type_t alert_type;     //PR_COMMAND
    pr_cmd_t command_type;          //PR_CMD_FWU_START, PR_CMD_FWU_CANCEL
    char file_server_url[LIB_HTTP_MAX_URL_SIZE];
} pr_cmd_fwu_body_t;

typedef struct {
    pr_alert_type_t alert_type; //PR_COMMAND
    pr_cmd_t command_type;      //PR_CMD_RESTART_CHILD
    char component[PR_MAX_PROC_NAME_SIZE];
} pr_cmd_restart_body_t;

typedef struct {
    pr_alert_type_t alert_type; //PR_COMMAND
    pr_cmd_t command_type;      //PR_CMD_CLOUD_CONN
    char conn_string[LIB_HTTP_MAX_URL_SIZE];
    char device_id[LIB_HTTP_DEVICE_ID_SIZE];
    char auth_token[LIB_HTTP_AUTHENTICATION_STRING_SIZE];
} pr_cmd_cloud_body_t;

typedef struct {
    pr_alert_type_t alert_type;     //PR_COMMAND
    pr_cmd_t command_type;          //PR_CMD_STOP
} pr_cmd_stop_body_t;

typedef union {
    pr_alert_type_t alert_type;         //just to have ability to look at the alert typr
    pr_cmd_t cmd_type;                  //just to have access to command type.NB! valid only if alert_type = PR_COMMAND
    pr_error_alert_body_t error;
    pr_monitor_alert_body_t monitor;
    pr_watchdog_alert_body_t watchdog;
    pr_fwu_alert_body_t fwu;
    pr_url_alert_body_t url;
    pr_cmd_fwu_body_t cmd_fwu;
    pr_cmd_restart_body_t cmd_restart;
    pr_cmd_cloud_body_t cmd_cloud;
    pr_cmd_stop_body_t cmd_stop;
} pr_alert_t;

void pr_store_child_name(int child_name, const char* name);
const char* pr_chld_2_string(pr_child_t child_name);
//NB!!! not thread-protected!!!
pr_child_t pr_string_2_chld(const char* child_name);
//
//return 1 if the json doesn't contain alerts
int pr_is_command(const char* json);
//Return message type = PR_ERROR if sonething wrong
void pr_json_2_struct(pr_alert_t* alert, const char* json_string);
//
//Converts pr_alert_t into JSON string. Last two parameters are valid for alerts only!
int pr_struct_2_json(char* buf, size_t size, pr_alert_t msg, const char* deviceID, unsigned int seqNo);
//
//Make the JSON alert from valuable data
//
int pr_make_error_alert(const char* error_text, const char* deviceID, unsigned int seqNum, char* json, size_t size);
int pr_make_monitor_alert(pr_monitor_alert_t mat, const char* comp, const char* rsn, const char* deviceID, unsigned int seqNum, char* json, size_t size);
int pr_make_wd_alert(const char* component, const char* deviceID, unsigned int seqNum, char* json, size_t size);
int pr_make_fwu_alert(pr_fwu_alert_t fwua_type, const char* alert_txt, const char* deviceID, unsigned int seqNum, char* json, size_t size);
int pr_make_url_alert(pr_url_alert_t urla_type, const char* alert_txt, const char* deviceID, unsigned int seqNum, char* json, size_t size);
//fs_url valid only for fw_start command
int pr_make_cmd_fwu(pr_cmd_t cmd_type, const char* fs_url, char* json, size_t size);
int pr_make_cmd_restart(const char* component, char* json, size_t size);
int pr_make_cmd_cloud(const char* cs, const char* di, const char* at, char* json, size_t size);
int pr_make_cmd_stop(char* json, size_t size);
////////////////////////////////////////////////////////////////////////
//Enum range chackers
///////////////////////
int pr_child_t_range_check(int item);
int pr_alert_type_range_check(int item);
int pr_error_alert_t_range_check(int item);
int pr_monitor_alert_t_range_check(int item);
int pr_watchdog_alert_t_range_check(int item);
int pr_fwu_alert_t_range_check(int item);
int pr_url_alert_t_range_check(int item);
int pr_cmd_fwu_t_range_check(int item);
int pr_cmd_restart_t_range_check(int item);
int pr_cmd_cloud_t_range_check(int item);
int pr_cmd_stop_range_check(int item);

#endif //PRESTO_PR_COMMANDS_H
