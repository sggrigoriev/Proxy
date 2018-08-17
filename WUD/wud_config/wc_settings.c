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
 */
/*
    Created by gsg on 29/11/16.
*/
#include <assert.h>
#include <pthread.h>
#include <string.h>
#include <stdio.h>
#include "stdlib.h"

#include "cJSON.h"
#include "lib_http.h"
#include "pc_config.h"

#include "wc_defaults.h"
#include "wc_settings.h"

                /* Configuration file fields names */

#define WUD_LOG_NAME          "LOG_NAME"
#define WUD_LOG_REC_AMT       "LOG_REC_AMT"

#define WUD_LOG_LEVEL         "LOG_LEVEL"
    #define WUD_LL_TRACE_2        "TRACE_2"
    #define WUD_LL_TRACE_1        "TRACE_1"
    #define WUD_LL_DEBUG          "DEBUG"
    #define WUD_LL_WARNING        "WARNING"
    #define WUD_LL_INFO           "INFO"
    #define WUD_LL_ERROR          "ERROR"

#define WUD_QUEUES_REC_AMT          "QUEUES_REC_AMT"
#define WUD_REBOOT_BY_REQUEST       "REBOOT_BY_REQUEST"

#define WUD_WORKING_DIRECTORY       "WUD_WORKING_DIRECTORY"
#define WUD_COMM_PORT               "WUD_COMM_PORT"

#define WUD_CHILDREN_SHUTDOWN_TO_SEC    "CHILDREN_SHUTDOWN_TO_SEC"

#define WUD_FW_DOWNLOAD_FOLDER      "FW_DOWNLOAD_FOLDER"
#define WUD_FW_UPGRADE_FOLDER       "FW_UPGRADE_FOLDER"
#define WUD_FW_UPGRADE_FILE_NAME    "FW_UPGRADE_FILE_NAME"

#define WUD_AGENT_PROCESS_NAME      "AGENT_PROCESS_NAME"
#define WUD_AGENT_BINARY_NAME       "AGENT_BINARY_NAME"
#define WUD_AGENT_RUN_PARAMETERS    "AGENT_RUN_PARAMETERS"
#define WUD_AGENT_WORKING_DIRECTORY "AGENT_WORKING_DIRECTORY"
#define WUD_AGENT_WD_TIMEOUT_SEC    "AGENT_WD_TIMEOUT_SEC"

#define WUD_PROXY_PROCESS_NAME      "PROXY_PROCESS_NAME"
#define WUD_PROXY_BINARY_NAME       "PROXY_BINARY_NAME"
#define WUD_PROXY_RUN_PARAMETERS    "PROXY_RUN_PARAMETERS"
#define WUD_PROXY_WORKING_DIRECTORY "PROXY_WORKING_DIRECTORY"
#define WUD_PROXY_WD_TIMEOUT_SEC    "PROXY_WD_TIMEOUT_SEC"

#define WUD_MONITORING_TO_SEC       "WUD_MONITORING_TO_SEC"
#define WUD_DELAY_BEFORE_START_SEC  "WUD_DELAY_BEFORE_START_SEC"

#define WUD_CURLOPT_CAINFO          "CURLOPT_CAINFO"
#define WUD_CURLOPT_SSL_VERIFYPEER  "CURLOPT_SSL_VERIFYPEER"

/****************************************************************************************
 Config values in memory. Loaded from the configuration file or taken from defaults if the file does not contain the setting
*/
static char             log_name[WC_MAX_PATH];
static unsigned int     log_rec_amt;
static log_level_t      log_level;

static unsigned int     queues_rec_amt;
static unsigned int     reboot_by_request;

static char working_dir[WC_MAX_PATH];
static unsigned int wud_port;
static unsigned int children_to_sec;

static char fw_download_folder[WC_MAX_PATH];
static char fw_upgrade_folder[WC_MAX_PATH];
static char fw_upgrade_file_name[WC_MAX_PATH];

static char         agent_process_name[WC_MAX_PROCESS_NAME_LEN];
static char         agent_binary_name[WC_MAX_PATH];
static char**       agent_run_parameters;
static char         agent_working_directory[WC_MAX_PATH];
static unsigned int agent_wd_timeout_sec;

static char         proxy_process_name[WC_MAX_PROCESS_NAME_LEN];
static char         proxy_binary_name[WC_MAX_PATH];
static char**       proxy_run_parameters;
static char         proxy_working_directory[WC_MAX_PATH];
static unsigned int proxy_wd_timeout_sec;

static unsigned int wud_monitoring_timeout_sec;
static unsigned int wud_delay_before_start_sec;

static int          curlopt_ssl_verify_peer;
static char         curlopt_ca_info[WC_MAX_PATH];

static char conf_fname[WC_MAX_PATH];

/* The data received from Proxy */
static char     proxy_device_id[LIB_HTTP_DEVICE_ID_SIZE] = "";
static char     cloud_url[LIB_HTTP_MAX_URL_SIZE] = "";
static char     proxy_auth_token[LIB_HTTP_AUTHENTICATION_STRING_SIZE] = "";
static char     firmware_version[LIB_HTTP_FW_VERSION_SIZE] = "";

static char** null_list = {NULL};   /* used for arg lists initiation - Proxy & Agent start parameters */

/*******************************************************************
 Local data
*/
/* Settings guard. Used in all "thread-protected" functions */
static pthread_mutex_t local_mutex = PTHREAD_MUTEX_INITIALIZER;

/* Indicates are defaults set 1 if set 0 if not */
static int initiated = 0;

/*******************************************************************
 Local functions declaration
*/

/*********************************************************************
 * Initiates config variables by defult values
 */
static void initiate_defaults();

/*********************************************************************
 * Copy log_level_t value (see pu_logger.h) of field_name of cgf object into uint_setting.
 * @param cfg           - parsed configuration
 * @param field_name    - log level field name in configuration fie
 * @param llt_setting   - the log level value converted into log_level_t received
 */
static void getLLTValue(cJSON* cfg, const char* field_name, log_level_t* llt_setting);

/*Copy char* argv[] into the setting. The last pointer is NULL. If empty or not found - NULL
NB! memory allocated here!
*********************************************************************
 * Copy char* argv[] into the setting. The last pointer is NULL. If empty or not found - NULL
 * @param cfg           - parsed configuration
 * @param field_name    - run parameters field name
 * @param setting       - NULL-terminated parameters list.
 */
static void getParTValue(cJSON* cfg, const char* field_name, char*** setting);

#define WC_RET(a,b) return (!initiated)?a:b
/************************************************************************
    Public functions

    Set of "get" functions to make an access to settings for Presto modules

*/

const char* wc_getLogFileName() {
    WC_RET(WC_DEFAULT_LOG_NAME, log_name);
}

/*Return max capacity of log file */
unsigned int wc_getLogRecordsAmt() {
    WC_RET(WC_DEFAULT_LOG_RECORDS_AMT, log_rec_amt);
}

log_level_t wc_getLogVevel() {
    WC_RET(WC_DEFAULT_LOG_LEVEL, log_level);
}

unsigned int wc_getQueuesRecAmt() {
    WC_RET(WC_DEFAULT_QUEUES_REC_AMT, queues_rec_amt);
}

unsigned int wc_getRebootByRequest() {
    WC_RET(WD_DEFAULT_REBOOT_BY_REQUEST, reboot_by_request);
}

const char* wc_getWorkingDir() {
    WC_RET(WUD_DEFAULT_WORKING_DIRECTORY, working_dir);
}

unsigned int wc_getWUDPort() {
    WC_RET(WUD_DEFAULT_COMM_PORT, wud_port);
}

unsigned int wc_getChildrenShutdownTO() {
    WC_RET(WUD_DEFAULT_CHILDREN_SHUTDOWN_TO_SEC, children_to_sec);
};

const char* wc_getFWDownloadFolder() {
    WC_RET(WUD_DEFAULT_FW_DOWNLOAD_FOLDER, fw_download_folder);
}

const char* wc_getFWUpgradeFolder() {
    WC_RET(WUD_DEFAULT_FW_UPGRADE_FOLDER, fw_upgrade_folder);
}

const char* wc_getFWUpgradeFileName() {
    WC_RET(WUD_FW_UPGRADE_FILE_NAME, fw_upgrade_file_name);
}

const char* wc_getAgentProcessName() {
    WC_RET(WUD_DEFAULT_AGENT_PROCESS_NAME, agent_process_name);
}

const char* wc_getAgentBinaryName() {
    WC_RET(WUD_DEFAULT_AGENT_BINARY_NAME, agent_binary_name);
}

char* const* wc_getAgentRunParameters() {
    WC_RET(null_list, agent_run_parameters);
}

const char* wc_getAgentWorkingDirectory() {
    WC_RET(WUD_DEFAULT_AGENT_WORKING_DIRECTORY, agent_working_directory);
}

unsigned int wc_getAgentWDTimeoutSec() {
    WC_RET(WUD_DEFAULT_AGENT_WD_TIMEOUT_SEC, agent_wd_timeout_sec);
}

const char* wc_getProxyProcessName() {
    WC_RET(WUD_DEFAULT_PROXY_PROCESS_NAME, proxy_process_name);
}

const char* wc_getProxyBinaryName() {
    WC_RET(WUD_DEFAULT_PROXY_BINARY_NAME, proxy_binary_name);
}

char* const* wc_getProxyRunParameters() {
    WC_RET(null_list, proxy_run_parameters);
}

const char* wc_getProxyWorkingDirectory() {
    WC_RET(WUD_DEFAULT_PROXY_WORKING_DIRECTORY, proxy_working_directory);
}

unsigned int wc_getProxyWDTimeoutSec() {
    WC_RET(WUD_DEFAULT_PROXY_WD_TIMEOUT_SEC, proxy_wd_timeout_sec);
}

unsigned int wc_getWUDMonitoringTO() {
    WC_RET(WUD_DEFAULT_MONITORING_TO_SEC, wud_monitoring_timeout_sec);
}

unsigned int wc_getWUDDelay() {
    WC_RET(WUD_DEFAILT_DELAY_BEFORE_START_SEC, wud_delay_before_start_sec);
}

const char* wc_getCurloptCAInfo() {
    return curlopt_ca_info;
}
int wc_getCurloptSSLVerifyPeer() {
    return curlopt_ssl_verify_peer;
}


/*********************************************************************************
    Thread-protected functions
*/

#define WC_ERR()    fprintf(stderr, "WUD: Default value will be used instead\n")

int wc_load_config(const char* cfg_file_name) {
    cJSON* cfg = NULL;
    assert(cfg_file_name);

    pthread_mutex_lock(&local_mutex);

    strcpy(conf_fname, cfg_file_name);

    initiate_defaults();
    if(cfg = load_file(cfg_file_name), cfg == NULL) {
        pthread_mutex_unlock(&local_mutex);
        return 0;
    }
/* Now load data */
    if(!getStrValue(cfg, WUD_LOG_NAME, log_name, sizeof(log_name)))                                                 WC_ERR();
    if(!getUintValue(cfg, WUD_LOG_REC_AMT, &log_rec_amt))                                                           WC_ERR();
    if(!getUintValue(cfg, WUD_QUEUES_REC_AMT, &queues_rec_amt))                                                     WC_ERR();

    getUintValue(cfg, WUD_REBOOT_BY_REQUEST, &reboot_by_request);

    if(!getStrValue(cfg, WUD_WORKING_DIRECTORY, working_dir, sizeof(working_dir)))                                  WC_ERR();
    if(!getUintValue(cfg, WUD_COMM_PORT, &wud_port))                                                                WC_ERR();
    if(!getUintValue(cfg, WUD_CHILDREN_SHUTDOWN_TO_SEC, &children_to_sec))                                          WC_ERR();
    if(!getStrValue(cfg, WUD_FW_DOWNLOAD_FOLDER, fw_download_folder, sizeof(fw_download_folder)))                   WC_ERR();
    if(!getStrValue(cfg, WUD_FW_UPGRADE_FOLDER, fw_upgrade_folder, sizeof(fw_upgrade_folder)))                      WC_ERR();
    if(!getStrValue(cfg, WUD_FW_UPGRADE_FILE_NAME, fw_upgrade_file_name, sizeof(fw_upgrade_file_name)))             WC_ERR();

    if(!getStrValue(cfg, WUD_AGENT_PROCESS_NAME, agent_process_name, sizeof(agent_process_name)))                   WC_ERR();
    if(!getStrValue(cfg, WUD_AGENT_BINARY_NAME, agent_binary_name, sizeof(agent_binary_name)))                      WC_ERR();
    if(!getStrValue(cfg, WUD_AGENT_WORKING_DIRECTORY, agent_working_directory, sizeof(agent_working_directory)))    WC_ERR();
    if(!getUintValue(cfg, WUD_AGENT_WD_TIMEOUT_SEC, &agent_wd_timeout_sec))                                         WC_ERR();

    if(!getStrValue(cfg, WUD_PROXY_PROCESS_NAME, proxy_process_name, sizeof(proxy_process_name)))                   WC_ERR();
    if(!getStrValue(cfg, WUD_PROXY_BINARY_NAME, proxy_binary_name, sizeof(proxy_process_name)))                     WC_ERR();
    if(!getStrValue(cfg, WUD_PROXY_WORKING_DIRECTORY, proxy_working_directory, sizeof(proxy_working_directory)))    WC_ERR();
    if(!getUintValue(cfg, WUD_PROXY_WD_TIMEOUT_SEC, &proxy_wd_timeout_sec))                                         WC_ERR();

    if(!getUintValue(cfg, WUD_MONITORING_TO_SEC, &wud_monitoring_timeout_sec))                                      WC_ERR();
    if(!getUintValue(cfg, WUD_DELAY_BEFORE_START_SEC, &wud_delay_before_start_sec))                                 WC_ERR();
    if(!getUintValue(cfg, WUD_CURLOPT_SSL_VERIFYPEER, (unsigned int* )&curlopt_ssl_verify_peer))                    WC_ERR();
    if(!getStrValue(cfg, WUD_CURLOPT_CAINFO, curlopt_ca_info, sizeof(curlopt_ca_info)))                             WC_ERR();

    getLLTValue(cfg, WUD_LOG_LEVEL, &log_level);
    getParTValue(cfg, WUD_AGENT_RUN_PARAMETERS, &agent_run_parameters);
    getParTValue(cfg, WUD_PROXY_RUN_PARAMETERS, &proxy_run_parameters);

    cJSON_Delete(cfg);

    pthread_mutex_unlock(&local_mutex);
    return 1;
}

void wc_getDeviceID(char* di, size_t size) {
    pthread_mutex_lock(&local_mutex);
    strncpy(di, proxy_device_id, size-1);
    pthread_mutex_unlock(&local_mutex);
}

void wc_setDeviceID(const char* di) {
    pthread_mutex_lock(&local_mutex);
    strncpy(proxy_device_id, di, sizeof(proxy_device_id)-1);
    pthread_mutex_unlock(&local_mutex);
}

void wc_getURL(char* url, size_t size) {
    pthread_mutex_lock(&local_mutex);
    strncpy(url, cloud_url, size-1);
    pthread_mutex_unlock(&local_mutex);
}

void wc_setURL(const char* url) {
    pthread_mutex_lock(&local_mutex);
    strncpy(cloud_url, url, sizeof(cloud_url)-1);
    pthread_mutex_unlock(&local_mutex);
}

void wc_getAuthToken(char* at, size_t size) {
    pthread_mutex_lock(&local_mutex);
    strncpy(at, proxy_auth_token, size-1);
    pthread_mutex_unlock(&local_mutex);
}

void wc_setAuthToken(const char* at) {
    pthread_mutex_lock(&local_mutex);
    strncpy(proxy_auth_token, at, sizeof(proxy_auth_token)-1);
    pthread_mutex_unlock(&local_mutex);
}

void wc_getFWVersion(char* ver, size_t size) {
    pthread_mutex_lock(&local_mutex);
    strncpy(ver, firmware_version, size-1);
    pthread_mutex_unlock(&local_mutex);
}

void wc_setFWVersion(const char* ver) {
    pthread_mutex_lock(&local_mutex);
    strncpy(firmware_version, ver, sizeof(firmware_version)-1);
    pthread_mutex_unlock(&local_mutex);
}

/**********************************************************
    Local function implementation
*/

static void initiate_defaults() {
    initiated = 1;
    strcpy(log_name, WC_DEFAULT_LOG_NAME);
    log_rec_amt = WC_DEFAULT_LOG_RECORDS_AMT;
    log_level = WC_DEFAULT_LOG_LEVEL;

    reboot_by_request= WD_DEFAULT_REBOOT_BY_REQUEST;

    strcpy(working_dir, WUD_DEFAULT_WORKING_DIRECTORY);
    wud_port = WUD_DEFAULT_COMM_PORT;
    wud_delay_before_start_sec = WUD_DEFAILT_DELAY_BEFORE_START_SEC;

    strcpy(fw_download_folder, WUD_DEFAULT_FW_DOWNLOAD_FOLDER);
    strcpy(fw_upgrade_folder, WUD_DEFAULT_FW_UPGRADE_FOLDER);
    strcpy(fw_upgrade_file_name, WC_DEFAULT_FWU_FILE_NAME);

    strcpy(agent_process_name, WUD_DEFAULT_AGENT_PROCESS_NAME);
    strcpy(agent_binary_name, WUD_DEFAULT_AGENT_BINARY_NAME);
    agent_run_parameters = null_list;
    strcpy(agent_working_directory, WUD_DEFAULT_AGENT_WORKING_DIRECTORY);
    agent_wd_timeout_sec = WUD_DEFAULT_AGENT_WD_TIMEOUT_SEC;

    strcpy(proxy_process_name, WUD_DEFAULT_PROXY_PROCESS_NAME);
    strcpy(proxy_binary_name, WUD_DEFAULT_PROXY_BINARY_NAME);
    proxy_run_parameters = null_list;
    strcpy(proxy_working_directory, WUD_DEFAULT_PROXY_WORKING_DIRECTORY);
    proxy_wd_timeout_sec = WUD_DEFAULT_PROXY_WD_TIMEOUT_SEC;

    strcpy(conf_fname, WD_DEFAULT_CFG_FILE_NAME);

    curlopt_ca_info[0] = '\0';
    curlopt_ssl_verify_peer = WUD_DEFAULT_CURLOPT_SSL_VERIFYPEER;
}

static void getLLTValue(cJSON* cfg, const char* field_name, log_level_t* llt_setting) {
    char buf[10];
    if(!getStrValue(cfg, field_name, buf, sizeof(buf))) {
        fprintf(stderr, "Default will be used instead.\n");
    }
    else {
        if(!strcmp(buf, WUD_LL_DEBUG))  *llt_setting = LL_DEBUG;
        else if(!strcmp(buf, WUD_LL_WARNING)) *llt_setting = LL_WARNING;
        else if(!strcmp(buf, WUD_LL_INFO)) *llt_setting = LL_INFO;
        else if(!strcmp(buf, WUD_LL_ERROR)) *llt_setting = LL_ERROR;
        else if(!strcmp(buf, WUD_LL_TRACE_1)) *llt_setting = LL_TRACE_1;
        else if(!strcmp(buf, WUD_LL_TRACE_2)) *llt_setting = LL_TRACE_2;
        else
            fprintf(stderr, "Setting %s = %s. Posssible values are %s, %s, %s or %s. Default will be used instead\n",
                    field_name, buf, WUD_LL_DEBUG, WUD_LL_INFO,  WUD_LL_WARNING, WUD_LL_ERROR
            );
    }
}

static void getParTValue(cJSON* cfg, const char* field_name, char*** setting) {
    if(!getCharArray(cfg, field_name, setting)) {
        fprintf(stderr, "Default will be used instead.\n");
    }
}
