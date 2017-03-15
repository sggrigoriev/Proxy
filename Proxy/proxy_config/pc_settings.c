/*
 *  Copyright 2017 People Power Company
 *
 *  This code was developed with funding from People Power Company
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http:www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
*/
/*
     Created by gsg on 29/10/16.
*/
#include <assert.h>
#include <pthread.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "cJSON.h"
#include "pc_config.h"

#include "pc_defaults.h"
#include "pc_settings.h"

/************************************************************************
    Config file fields names
*/

#define PROXY_LOG_NAME          "LOG_NAME"
#define PROXY_LOG_REC_AMT       "LOG_REC_AMT"

#define PROXY_LOG_LEVEL         "LOG_LEVEL"
    #define PROXY_LL_DEBUG          "DEBUG"
    #define PROXY_LL_WARNING        "WARNING"
    #define PROXY_LL_INFO           "INFO"
    #define PROXY_LL_ERROR          "ERROR"

#define PROXY_PROCESS_NAME      "PROXY_PROCESS_NAME"

#define PROXY_DEVICE_ID         "DEVICE_ID"
#define PROXY_DEVICE_TYPE       "DEVICE_TYPE"

#define PROXY_AUTH_TOKEN_FILE_NAME  "AUTH_TOKEN_FILE_NAME"

#define PROXY_MAIN_CLOUD_URL    "MAIN_CLOUD_URL"

#define PROXY_CLOUD_URL_REQ_TO_HRS  "CLOUD_URL_REQ_TO_HRS"
#define PROXY_FW_VER_SEND_TO_HRS    "FW_VER_SEND_TO_HRS"

#define PROXY_UPLOAD_TO_SEC     "UPLOAD_TO_SEC"
#define PROXY_QUEUES_REC_AMT    "QUEUES_REC_AMT"
#define PROXY_AGENT_PORT        "AGENT_PORT"

#define PROXY_WUD_PORT          "WUD_PORT"
#define PROXY_WATCHDOG_TO_SEC   "WATCHDOG_TO_SEC"

/********************************************************************
    Config values saved in memory
*/
static char             proxy_name[PROXY_MAX_PROC_NAME];
static char             log_name[PROXY_MAX_PATH];
static unsigned int     log_rec_amt;
static log_level_t      log_level;

static char             auth_token[PROXY_MAX_ACTIVATION_TOKEN_SIZE];
static char             auth_token_file_name[PROXY_MAX_PATH];
static char             device_id[PROXY_DEVICE_ID_SIZE];
static unsigned int     device_type;
static char             cloud_url[PROXY_MAX_PATH];
static char             main_cloud_url[PROXY_MAX_PATH];

static unsigned int     cloud_url_req_to_hrs;
static unsigned int     fw_ver_sending_to_hrs;

static unsigned int     long_get_to;
static unsigned int     queue_rec_amt;
static unsigned int     agent_port;
static unsigned int     WUD_port;
static unsigned int     proxy_wd_to;

static char             fw_version[DEFAULT_FW_VERSION_SIZE];

static char conf_fname[PROXY_MAX_PATH];


/*****************************************************************************************************
    Local functions
*/

/* Settings guard. Used in all "thread-protected" functions */
static pthread_mutex_t local_mutex = PTHREAD_MUTEX_INITIALIZER;

/* Indicates are defaults set 1 if set 0 if not */
static int initiated = 0;

/* Initiates config variables by defult values */
static void initiate_defaults();

/* Copy log_level_t value (see pu_logger.h) of field_name of cgf object into uint_setting. */
/* Copy default value in case of absence of the field in the object */
static void getLLTValue(cJSON* cfg, const char* field_name, log_level_t* llt_setting);

/* Reads auth token from the file. If no token - set it as an empty string */
/* no thread protection! */
static void read_auth_token(const char* at_file_name, char* at, size_t size);

/* Return 0 if error */
static int save_auth_token(const char* at_file_name, const char* new_at);

/***************************************************************************************/
#define PC_RET(a,b) return (!initiated)?a:b

/* Set of "get" functions to make an access to settings for Presto modules */

/* Return LOG-file name */
const char* pc_getLogFileName() {
    PC_RET(DEFAULT_LOG_NAME, log_name);
}

/* Return max capacity of log file */
size_t pc_getLogRecordsAmt() {
    PC_RET(DEFAULT_LOG_RECORDS_AMT, log_rec_amt);
}

/* Return the min log level to be stored in LOG_FILE */
/* LL_DEBUG<LL_WARNING<LL_INFO<LL_ERROR */
log_level_t pc_getLogVevel() {
    PC_RET(DEFAULT_LOG_LEVEL, log_level);
}

/* Return the timeout in seconds for "long get" made by Presto to listen the Cloud messages. */
unsigned int pc_getLongGetTO() {
    PC_RET(DEFAULT_UPLOAD_TIMEOUT_SEC, long_get_to);
}

/* Return the port# for communications with the Agent */
unsigned int pc_getAgentPort() {
    PC_RET(DEFAULT_AGENT_PORT, agent_port);
}

/* Return max amount of records kept in Presto queues */
size_t pc_getQueuesRecAmt() {
    PC_RET(DEFAULT_QUEUE_RECORDS_AMT, queue_rec_amt);
}

/* Return the Presto device type.(31 for now...) */
unsigned int pc_getProxyDeviceType() {
    PC_RET(DEFAULT_PROXY_DEV_TYPE, device_type);
}

/* Return port for watchdogging */
unsigned int pc_getWUDPort() {
    PC_RET(DEFAULT_WUD_PORT, WUD_port);
}

/* Get the Proxy process name - used in watchdog messages */
const char* pc_getProxyName() {
    PC_RET(PROXY_PROCESS_NAME, proxy_name);
}

/* Get the watchdog sending period */
unsigned int pc_getProxyWDTO() {
    PC_RET(DEFAULT_PROXY_WATCHDOG_TO_SEC, proxy_wd_to);
}

/* Get the contact cloud URL request period in seconds */
unsigned int pc_getCloudURLTOHrs() {
    PC_RET(DEFAULT_CLOUD_URL_REQ_TO_HRS, cloud_url_req_to_hrs);
}

/* Get the period of firmware version sendinf to the cloud by Proxy */
unsigned int pc_getFWVerSendToHrs() {
    PC_RET(DEFAULT_FW_VER_SENDING_TO_HRS, fw_ver_sending_to_hrs);
}

/***********************************************************************
    Thread-protected functions
*/
/* Load configuration settings from cfg_file_name into the memory. All settings not mentioned in cfg_file_name */
/* got default values (see pc_defaults.h) */
/*  return 1 if success, 0 if not */
int pc_load_config(const char* cfg_file_name) {
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
    if(!getStrValue(cfg, PROXY_LOG_NAME, log_name, sizeof(log_name)))                           fprintf(stderr, "Default value will be used instead\n");
    if(!getUintValue(cfg, PROXY_LOG_REC_AMT, &log_rec_amt))                                     fprintf(stderr, "Default value will be used instead\n");
    getLLTValue(cfg, PROXY_LOG_LEVEL, &log_level);

    if(getStrValue(cfg, PROXY_DEVICE_ID, device_id, sizeof(device_id)))                         fprintf(stderr, "DeviceID get from config file\n");

    if(getStrValue(cfg, PROXY_AUTH_TOKEN_FILE_NAME, auth_token_file_name, sizeof(auth_token_file_name))) fprintf(stderr, "Default value will be used instead\n");
    read_auth_token(auth_token_file_name, auth_token, sizeof(auth_token));

    if(!getUintValue(cfg, PROXY_DEVICE_TYPE, &device_type))                                     fprintf(stderr, "Default value will be used instead\n");
    if(!getStrValue(cfg, PROXY_MAIN_CLOUD_URL, main_cloud_url, sizeof(cloud_url)))              fprintf(stderr, "Default value will be used instead\n");
    if(!getUintValue(cfg, PROXY_CLOUD_URL_REQ_TO_HRS, &cloud_url_req_to_hrs))                   fprintf(stderr, "Default value will be used instead\n");
    if(!getUintValue(cfg, PROXY_FW_VER_SEND_TO_HRS, &fw_ver_sending_to_hrs))                    fprintf(stderr, "Default value will be used instead\n");

    if(!getUintValue(cfg, PROXY_UPLOAD_TO_SEC, &long_get_to))                                   fprintf(stderr, "Default value will be used instead\n");
    if(!getUintValue(cfg, PROXY_QUEUES_REC_AMT, &queue_rec_amt))                                fprintf(stderr, "Default value will be used instead\n");
    if(!getUintValue(cfg, PROXY_AGENT_PORT, &agent_port))                                       fprintf(stderr, "Default value will be used instead\n");
    if(!getUintValue(cfg, PROXY_WUD_PORT, &WUD_port))                                           fprintf(stderr, "Default value will be used instead\n");
    if(!getStrValue(cfg, PROXY_PROCESS_NAME, proxy_name, sizeof(proxy_name)))                   fprintf(stderr, "Default value will be used instead\n");
    if(!getUintValue(cfg, PROXY_WATCHDOG_TO_SEC, &proxy_wd_to))                                 fprintf(stderr, "Default value will be used instead\n");

    cJSON_Delete(cfg);

    pthread_mutex_unlock(&local_mutex);
    return 1;
}

/* Copy to the ret the clour uprl string */
/* return empty string if no value or the value > max_len */
void pc_getCloudURL(char* ret, size_t size) {
    assert(ret); assert(size);
    pthread_mutex_lock(&local_mutex);

    strncpy(ret, cloud_url, size);
    ret[size-1] = '\0';

    pthread_mutex_unlock(&local_mutex);
}

/* Copy the the ret the default cloud contact point string */
/* Copy empty string if the value > max_len */
void pc_getMainCloudURL(char* ret, size_t size) {
    assert(ret); assert(size);
    pthread_mutex_lock(&local_mutex);

    strncpy(ret, main_cloud_url, size);
    ret[size-1] = '\0';

    pthread_mutex_unlock(&local_mutex);
}

/* Copy to the ret the auth token string. */
/* Copy empty string if the value > max_len */
void pc_getAuthToken(char* ret, size_t size) {
    assert(ret); assert(size);
    pthread_mutex_lock(&local_mutex);

    strncpy(ret, auth_token, size);
    ret[size-1] = '\0';

    pthread_mutex_unlock(&local_mutex);
}

/* Copy to the ret the device address (deviceID) string. */
/* Copy empty string if the value > max_len */
void pc_getProxyDeviceID(char* ret, size_t size) {
    assert(ret); assert(size);
    pthread_mutex_lock(&local_mutex);

    strncpy(ret, device_id, size);
    ret[size-1] = '\0';

    pthread_mutex_unlock(&local_mutex);
}

/* Update the current value in memory */
/* Return 1 of success, return 0 if not */
int pc_saveAuthToken(const char* new_at) {

    pthread_mutex_lock(&local_mutex);

    strncpy(auth_token, new_at, sizeof(auth_token)-1);
    int ret = save_auth_token(auth_token_file_name, auth_token);

    pthread_mutex_unlock(&local_mutex);
    return ret;
}

/* Update the current value in memory */
/* Return 1 of success, return 0 if not */
int pc_saveProxyDeviceID(const char* new_da) {

    pthread_mutex_lock(&local_mutex);

    strncpy(device_id, new_da, sizeof(device_id));

    pthread_mutex_unlock(&local_mutex);
    return 1;
}

/* Update/insert new default Cloud connection point. Update the current value in memory as well */
/* Return 1 of success, return 0 if not */
int pc_saveMainCloudURL(const char* new_main_url) {
    int ret;

    pthread_mutex_lock(&local_mutex);

    ret = saveStrValue("pc_saveMainCloudURL", conf_fname, PROXY_MAIN_CLOUD_URL, new_main_url, main_cloud_url, sizeof(main_cloud_url));

    pthread_mutex_unlock(&local_mutex);
    return ret;
}

/* Update the current value in memory only! */
/* Return 1 of success, return 0 if not */
int pc_saveCloudURL(const char* new_url) {

    pthread_mutex_lock(&local_mutex);

    strncpy(cloud_url, new_url, sizeof(cloud_url)-1);

    pthread_mutex_unlock(&local_mutex);
    return 1;
}

/* Update/insert the port for communicating with the Agent. Update the current value in memory as well */
/* Return 1 of success, return 0 if not */
int pc_saveAgentPort(unsigned int new_port) {
    int ret;
    pthread_mutex_lock(&local_mutex);

    ret = saveUintValue("pc_saveAgentPort", conf_fname, PROXY_AGENT_PORT, new_port, &agent_port);

    pthread_mutex_unlock(&local_mutex);
    return ret;
}

/* Update configuration file name. NB! Think twice before the call this function notat the very beginning of all Presto! */
/* Return 1 of success, return 0 if not */
int pc_saveCfgFileName(const char* new_file_name) {
    pthread_mutex_lock(&local_mutex);

    if(sizeof(conf_fname) < strlen(new_file_name)+1) return 0;

    strcpy(conf_fname, new_file_name);

    pthread_mutex_unlock(&local_mutex);
    return 1;
}

/* FW version set/get */
void pc_readFWVersion() {    /* Reads the version from DEFAULT_FW_VERSION_FILE */
    FILE* f = fopen(DEFAULT_FW_VERSION_FILE, "rt");
    if(!f) {
        fprintf(stderr, "Can't open %s: %d, %s. Firware version is undefined\n", DEFAULT_FW_VERSION_FILE, errno, strerror(errno));
        return;
    }
    size_t sz = fscanf(f,"%s",fw_version);
    if(!sz) {
        fprintf(stderr, "Can't read %s: %d, %s. Firware version is undefined\n", DEFAULT_FW_VERSION_FILE, errno, strerror(errno));
        return;
    }
}

void pc_getFWVersion(char* fw_ver, size_t size) {
    pthread_mutex_lock(&local_mutex);

    strncpy(fw_ver, fw_version, size-1);
    fw_ver[size-1] = '\0';

    pthread_mutex_unlock(&local_mutex);
}

/* Activation-related stuff */
int pc_existsProxyDeviceID() {
    return (initiated && strlen(device_id));
}

/************************************************************************************************
 * Local functions implementation
 */
static void initiate_defaults() {
    initiated = 1;
    strcpy(log_name, DEFAULT_LOG_NAME);
    log_rec_amt = DEFAULT_LOG_RECORDS_AMT;
    log_level = DEFAULT_LOG_LEVEL;

    auth_token[0] = '\0';
    strncpy(auth_token_file_name, DEFAULT_AUTH_TOKEN_FILE_NAME, sizeof(auth_token_file_name));
    device_id[0] = '\0';
    cloud_url[0] = '\0';
    strncpy(main_cloud_url, DEFAULT_MAIN_CLOUD_URL, sizeof(main_cloud_url));
    cloud_url_req_to_hrs = DEFAULT_CLOUD_URL_REQ_TO_HRS;

    long_get_to = DEFAULT_UPLOAD_TIMEOUT_SEC;
    queue_rec_amt = DEFAULT_QUEUE_RECORDS_AMT;
    agent_port = DEFAULT_AGENT_PORT;
    strncpy(fw_version, DEFAULT_FW_VERSION_NUM, DEFAULT_FW_VERSION_SIZE-1);
}

/*
 Get log_level_t value
    cfg           - poiner to cJSON object containgng configuration
    field_name    - JSON fileld name
    llt_setting   - returned value of field_name
*/
static void getLLTValue(cJSON* cfg, const char* field_name, log_level_t* llt_setting) {
    char buf[10];
    if(!getStrValue(cfg, field_name, buf, sizeof(buf))) {
        fprintf(stderr, "Default will be used instead.\n");
    }
    else {
        if(!strcmp(buf, PROXY_LL_DEBUG))  *llt_setting = LL_DEBUG;
        else if(!strcmp(buf, PROXY_LL_WARNING)) *llt_setting = LL_WARNING;
        else if(!strcmp(buf, PROXY_LL_INFO)) *llt_setting = LL_INFO;
        else if(!strcmp(buf, PROXY_LL_ERROR)) *llt_setting = LL_ERROR;
        else
            fprintf(stderr, "Setting %s = %s. Posssible values are %s, %s, %s or %s. Default will be used instead\n",
                   field_name, buf, PROXY_LL_DEBUG, PROXY_LL_INFO,  PROXY_LL_WARNING, PROXY_LL_ERROR
            );
    }
}

static void read_auth_token(const char* at_file_name, char* at, size_t size) {
    FILE* f = fopen(at_file_name, "r");
    if(!f) {
        fprintf(stderr, "File %s open error %d, %s. Auth token should be assigned!\n", at_file_name, errno, strerror(errno));
        goto on_err;
    }
    char* ptr = fgets(at, size, f);
    if(!ptr) {
        fprintf(stderr, "File %s read error %d, %s. Auth token should be assigned!\n", at_file_name, errno, strerror(errno));
        goto on_err;
    }
    fclose(f);
    return;
on_err:
    auth_token[0] = '\0';
    if(f) fclose(f);
    return;
}

static int save_auth_token(const char* at_file_name, const char* new_at) {
    FILE* f = fopen(at_file_name, "w");
    if(!f) return 0;
    fprintf(f, "%s", new_at);
    fclose(f);
    return 1;
}