//
// Created by gsg on 08/12/16.
//
// Contains Presto commands & alerts internal representation + conversion to/from JSON utilities
//
#ifndef PRESTO_PR_COMMANDS_H
#define PRESTO_PR_COMMANDS_H

#include <unistd.h>
#include <time.h>

#include "cJSON.h"

typedef enum {WA_PROC1, WA_PROC_2, WA_SIZE
} wa_child_t;
///////////////////////////////////////////////////////
typedef enum {PR_MSG_ERROR, PR_MONITOR_ALERT, PR__WATCHDOG_ALERT, PR_FWU_ALERT, PR_WUD_ALERT, PR_COMMAND, PR_MSG_SIZE
} pr_msg_type_t;
    typedef enum {PR_FW_UPGRADE_START, PR_FW_UPGRADE_CANCEL, PR_STOP, PR_RESTART_CHILD, PR_PRESTO_INFO,
                    PR_CMD_SIZE
    } pr_command_id_t;
    typedef enum {PR_FWU_INIT_FAILED, PR_FWU_DOWNLOAD_FAIL, PR_FWU_CHECK_FAIL, PR_FWU_MOVE_FAIL, PR_FWU_INIT_OK,
                    PR_FWU_DOWNLOAD_OK, PR_FWU_CHECK_OK, PR_FWU_MOVE_OK, PR_FWU_DOWNLOAD_BUSY, PR_FWU_UPGRADE_BUSY,
                    PR_FWU_CANCEL, PR_FWU_SIZE
    } pr_fwu_msg_t;
    typedef enum {PR_WUD_START, PR_WUD_CHILD_RESTART, PR_WUD_REBOOT, PR_WUD_SIZE
    } pr_wud_msg_t;
///////////////////////////////////////////////////////
typedef enum {PR_MON_ALERT, PR_MON_ALERT_SIZE
} pr_mon_alert_t;

typedef char* pr_err_body_t;

typedef char* pr_fw_start_body_t;

typedef struct {
    char* cloud_conn_string;
    char* device_id;
    char* activation_token;
} pr_presto_info_body_t;

typedef union {
    pr_command_id_t         cmd;
    pr_fw_start_body_t      fw_conn_str;
    wa_child_t              restart_chld;
    pr_presto_info_body_t   presto_info;
} pr_command_t;

const char* pr_chld_2_string(wa_child_t child_name);
wa_child_t pr_string_2_chld(const char* child_name);
void pr_store_child_name(int child_name, const char* name);

typedef struct {
    unsigned int    alertId;                   //unique ID
    pr_mon_alert_t  alertType;
    time_t          alertTimestamp;                  //time_t
    char*           proc_name;                        // process name
    char*           errorCode;                        // diagnostics
} pr_mon_alert_body_t;

typedef struct {
    time_t  timestamp;
    pr_fwu_msg_t alert_type;
    char*   error;
} pr_fwu_alert_body_t;

typedef struct {
    time_t  timestamp;
    char*   proc_name;                        // process name
    char*   error;
} pr_wud_alert_body_t;

typedef union {
    pr_msg_type_t       message_type;
    char*               deviceID;
    pr_command_t        command;
    char*               err_text;
    pr_mon_alert_body_t monitor_alert;
    char*               watchdog_alert;
    pr_fwu_alert_body_t fw_upgrade_alert;
    pr_wud_alert_body_t wud_alert;
} pr_message_t;

//Return message type = PR_ERROR if sonething wrong
pr_message_t pr_json_2_msg(const char* json_string);
void pr_erase_msg(pr_message_t msg);
int pr_make_message(char** buf, pr_message_t msg);
//
int pr_make_wud_alert(char** buf, pr_wud_msg_t wud_msg_type, const char* deviceID, const char* proc_name);
int pr_make_fwu_alert(char** buf, pr_fwu_msg_t fwu_alert, const char* deviceID, const char* proc_name);
//
//support functions
int wa_child_t_range_check(int item);
int pr_command_id_t_range_check(int item);
int pr_msg_type_t_range_check(int item);
int pr_fwu_msg_t_range_check(int item);
int pr_wud_msg_t_range_check(int item);
int pr_mon_alert_t_range_check(int item);








#endif //PRESTO_PR_COMMANDS_H
