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
#include <pr_ptr_list.h>

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
    #define PROXY_LL_TRACE_2        "TRACE_2"
    #define PROXY_LL_TRACE_1        "TRACE_1"
    #define PROXY_LL_DEBUG          "DEBUG"
    #define PROXY_LL_WARNING        "WARNING"
    #define PROXY_LL_INFO           "INFO"
    #define PROXY_LL_ERROR          "ERROR"

#define PROXY_PROCESS_NAME      "PROXY_PROCESS_NAME"

#define PROXY_DEVICE_ID         "DEVICE_ID"
#define PROXY_DEVICE_TYPE       "DEVICE_TYPE"

#define PROXY_AUTH_TOKEN_FILE_NAME  "AUTH_TOKEN_FILE_NAME"

#define PROXY_MAIN_CLOUD_URL_FILE_NAME "MAIN_CLOUD_URL_FILE_NAME"

#define PROXY_CLOUD_URL_REQ_TO_HRS  "CLOUD_URL_REQ_TO_HRS"
#define PROXY_FW_VER_SEND_TO_HRS    "FW_VER_SEND_TO_HRS"

#define PROXY_QUEUES_REC_AMT    "QUEUES_REC_AMT"
#define PROXY_AGENT_PORT        "AGENT_PORT"
#define PROXY_LISTEN_IP         "PROXY_LISTEN_IP"

#define PROXY_WUD_PORT          "WUD_PORT"
#define PROXY_WATCHDOG_TO_SEC   "WATCHDOG_TO_SEC"

#define PROXY_SET_SSL_FOR_URL_REQUEST   "SET_SSL_FOR_URL_REQUEST"
#define PROXY_CURLOPT_CAINFO            "CURLOPT_CAINFO"
#define PROXY_CURLOPT_SSL_VERIFYPEER    "CURLOPT_SSL_VERIFYPEER"

#define PROXY_DEVICE_ID_PREFIX          "DEVICE_ID_PREFIX"
#define PROXY_REBOOT_IF_CLOUD_REJECTS   "REBOOT_IF_CLOUD_REJECTS"

#define PROXY_CLOUD_CONN_TIMEOUT        "CLOUD_CONN_TIMEOUT"
#define PROXY_CLOUD_POST_ATTEMPTS       "CLOUD_POST_ATTEMPTS"
#define PROXY_LONG_GET_KEEPALIVE_TO     "LONG_GET_KEEPALIVE_TO"
#define PROXY_LONG_GET_TO               "LONG_GET_TO"
#define PROXY_ALLOWED_DOMAINS           "ALLOWED_DOMAINS"


/********************************************************************
    Config values saved in memory
*/
static char             proxy_name[PROXY_MAX_PROC_NAME] = {0};
static char             log_name[PROXY_MAX_PATH] = {0};
static unsigned int     log_rec_amt = 0;
static log_level_t      log_level = 0;

static char             auth_token[PROXY_MAX_ACTIVATION_TOKEN_SIZE] = {0};
static char             auth_token_file_name[PROXY_MAX_PATH] = {0};
static char             device_id[PROXY_DEVICE_ID_SIZE] = {0};
static unsigned int     device_type = 0;
static char             cloud_url[PROXY_MAX_PATH] = {0};
static char             main_cloud_url[PROXY_MAX_PATH] = {0};
static char             main_cloud_url_file_name[PROXY_MAX_PATH] = {0};

static unsigned int     cloud_url_req_to_hrs = 0;
static unsigned int     fw_ver_sending_to_hrs = 0;

static unsigned int     queue_rec_amt = 0;
static unsigned int     agent_port = 0;
static unsigned int     WUD_port = 0;
static unsigned int     proxy_wd_to = 0;
static char             proxy_listen_ip[16] = {0};

static unsigned int     set_ssl_for_url_request = 0;
static int              curlopt_ssl_verify_peer = 0;
static char             curlopt_ca_info[PROXY_MAX_PATH] = {0};

static char             device_id_prefix[20] = {0};
static int              reboot_if_cloud_rejects = 0;

static unsigned int     cloud_conn_timeout = 0;
static unsigned int     cloud_post_attempts = 0;
static unsigned int     long_get_keepalive_to = 0;
static unsigned int     long_get_to = 0;

static char             fw_version[DEFAULT_FW_VERSION_SIZE] = {0};

static char conf_fname[PROXY_MAX_PATH] = {0};

static char** allowed_domains = NULL;


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

/***************************************************************************************/
#define PCS_ERR fprintf(stderr, "PROXY: Default value will be used instead.\n")

/* Set of "get" functions to make an access to settings for Presto modules */

/* Return LOG-file name */
const char* pc_getLogFileName() {
    return log_name;
}

/* Return max capacity of log file */
size_t pc_getLogRecordsAmt() {
    return log_rec_amt;
}

/* Return the min log level to be stored in LOG_FILE */
/* LL_DEBUG<LL_WARNING<LL_INFO<LL_ERROR */
log_level_t pc_getLogVevel() {
    return log_level;
}

/* Return the port# for communications with the Agent */
unsigned int pc_getAgentPort() {
    return agent_port;
}

/* Return the listen IP address */
const char*     pc_getProxyListenIP() {
    return proxy_listen_ip;
}

/* Return max amount of records kept in Presto queues */
size_t pc_getQueuesRecAmt() {
    return queue_rec_amt;
}

/* Return the Presto device type.(31 for now...) */
unsigned int pc_getProxyDeviceType() {
    return device_type;
}

/* Return port for watchdogging */
unsigned int pc_getWUDPort() {
    return WUD_port;
}

/* Get the Proxy process name - used in watchdog messages */
const char* pc_getProxyName() {
    return proxy_name;
}

/* Get the watchdog sending period */
unsigned int pc_getProxyWDTO() {
    return proxy_wd_to;
}

/* Get the contact cloud URL request period in seconds */
unsigned int pc_getCloudURLTOHrs() {
    return cloud_url_req_to_hrs;
}

/* Get the period of firmware version sendinf to the cloud by Proxy */
unsigned int pc_getFWVerSendToHrs() {
    return fw_ver_sending_to_hrs;
}

int pc_getSetSSLForCloudURLRequest() {
    return set_ssl_for_url_request;
}

int pc_getCurloptSSPVerifyPeer() {
    return curlopt_ssl_verify_peer;
}

const char* pc_getCurloptCAInfo() {
    return curlopt_ca_info;
}

const char* pc_getProxyDeviceIDPrefix() {
    return device_id_prefix;
}

int pc_rebootIfCloudRejects() {
    return reboot_if_cloud_rejects;
}

unsigned int    pc_getCloudConnTimeout() {
    return cloud_conn_timeout;
}
unsigned int    pc_getCloudPostAttempts() {
    return cloud_post_attempts;
}
unsigned int    pc_getLongGetKeepaliveTO() {
    return long_get_keepalive_to;
}
unsigned int    pc_getLongGetTO() {
    return long_get_to;
}

static int ends_by(const char* str, const char* end) {
    if(!str || !end) return 1;
    int i=0;
    const char* p_str = str + strlen(str);
    const char* p_end = end + strlen(end);
    while(i < strlen(str)&&(i < strlen(end))) {
        if(*p_str != *p_end) return 0;
        p_str--; p_end--; i++;
    }
    return 1;
}
int pc_isAllowedURL(const char* url) {
    char** ptr = allowed_domains;
    while (*ptr) {
        if(ends_by(url, *ptr)) return 1;
        ptr++;
    }
    return 0;
}
char* const* pc_getAllowedDomainsList() {
    return allowed_domains;
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
    if(!getStrValue(cfg, PROXY_LOG_NAME, log_name, sizeof(log_name)))                           PCS_ERR;
    if(!getUintValue(cfg, PROXY_LOG_REC_AMT, &log_rec_amt))                                     PCS_ERR;
    getLLTValue(cfg, PROXY_LOG_LEVEL, &log_level);

    getStrValue(cfg, PROXY_DEVICE_ID, device_id, sizeof(device_id));

    if(!getStrValue(cfg, PROXY_AUTH_TOKEN_FILE_NAME, auth_token_file_name, sizeof(auth_token_file_name)))
        fprintf(stderr, "%s setting is not found. Default name will be used instead\n", PROXY_AUTH_TOKEN_FILE_NAME);

    if(!read_one_string_file(auth_token_file_name, auth_token, sizeof(auth_token), PROXY_AUTH_TOKEN_FILE_NAME)) {
        fprintf(stderr, "Auth Token will be set by the cloud\n");
    }

    if(!getUintValue(cfg, PROXY_DEVICE_TYPE, &device_type))                                     PCS_ERR;

    if(!getStrValue(cfg, PROXY_MAIN_CLOUD_URL_FILE_NAME, main_cloud_url_file_name, sizeof(main_cloud_url_file_name))) PCS_ERR;
    if(!read_one_string_file(main_cloud_url_file_name, main_cloud_url, sizeof(main_cloud_url), PROXY_MAIN_CLOUD_URL_FILE_NAME)) {
        PCS_ERR;
        if(device_type != PROXY_C1_DEVICE_TYPE) { /* Not a camera -> we should save default URL */
            strncpy(main_cloud_url, DEFAULT_MAIN_CLOUD_URL, sizeof(main_cloud_url) - 1);  /* Set default value */
            if (!save_one_string_file(main_cloud_url_file_name, main_cloud_url, PROXY_MAIN_CLOUD_URL_FILE_NAME))
                return 0;  /* All diagnostics inside */
        }
        else { /* Cam case -> empty string shouild be passed. URL will be set by QR code */
            main_cloud_url[0] = '\0';
        }
    }

    if(!getUintValue(cfg, PROXY_CLOUD_URL_REQ_TO_HRS, &cloud_url_req_to_hrs))                   PCS_ERR;
    if(!getUintValue(cfg, PROXY_FW_VER_SEND_TO_HRS, &fw_ver_sending_to_hrs))                    PCS_ERR;

    if(!getUintValue(cfg, PROXY_QUEUES_REC_AMT, &queue_rec_amt))                                PCS_ERR;
    if(!getUintValue(cfg, PROXY_AGENT_PORT, &agent_port))                                       PCS_ERR;
    if(!getUintValue(cfg, PROXY_WUD_PORT, &WUD_port))                                           PCS_ERR;
    if(!getStrValue(cfg, PROXY_LISTEN_IP, proxy_listen_ip, sizeof(proxy_listen_ip)))            PCS_ERR;
    if(!getStrValue(cfg, PROXY_PROCESS_NAME, proxy_name, sizeof(proxy_name)))                   PCS_ERR;
    if(!getUintValue(cfg, PROXY_WATCHDOG_TO_SEC, &proxy_wd_to))                                 PCS_ERR;
    if(!getUintValue(cfg, PROXY_SET_SSL_FOR_URL_REQUEST, &set_ssl_for_url_request))             PCS_ERR;
    if(!getUintValue(cfg, PROXY_CURLOPT_SSL_VERIFYPEER, (unsigned int* )&curlopt_ssl_verify_peer))          PCS_ERR;
    if(!getStrValue(cfg, PROXY_CURLOPT_CAINFO, curlopt_ca_info, sizeof(curlopt_ca_info)))       PCS_ERR;

    if(!getStrValue(cfg, PROXY_DEVICE_ID_PREFIX, device_id_prefix, sizeof(device_id_prefix)))   PCS_ERR;
    if(!getUintValue(cfg, PROXY_REBOOT_IF_CLOUD_REJECTS, (unsigned int *)(&reboot_if_cloud_rejects)))       PCS_ERR;

    if(!getUintValue(cfg, PROXY_CLOUD_CONN_TIMEOUT, (unsigned int *)(&cloud_conn_timeout)))     PCS_ERR;
    if(!getUintValue(cfg, PROXY_CLOUD_POST_ATTEMPTS, (unsigned int *)(&cloud_post_attempts)))   PCS_ERR;
    if(!getUintValue(cfg, PROXY_LONG_GET_KEEPALIVE_TO, &long_get_keepalive_to))                 PCS_ERR;
    if(!getUintValue(cfg, PROXY_LONG_GET_TO, &long_get_to))                                     PCS_ERR;

    char** tmp;
    if(!getCharArray(cfg, PROXY_ALLOWED_DOMAINS, &tmp))                                         PCS_ERR;
    else {
        if(allowed_domains) free(allowed_domains);
        allowed_domains = tmp;
    }

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
    int ret = save_one_string_file(auth_token_file_name, auth_token, PROXY_AUTH_TOKEN_FILE_NAME);

    pthread_mutex_unlock(&local_mutex);
    return ret;
}

/* Update the current value in memory */
/* Return 1 of success, return 0 if not */
int pc_saveProxyDeviceID(const char* new_da) {

    pthread_mutex_lock(&local_mutex);

    strncpy(device_id, new_da, sizeof(device_id)-1);

    pthread_mutex_unlock(&local_mutex);
    return 1;
}

/* Update/insert new default Cloud connection point. Update the current value in memory as well */
/* Return 1 of success, return 0 if not */
int pc_saveMainCloudURL(const char* new_main_url) {
    int ret;

    pthread_mutex_lock(&local_mutex);

    strncpy(main_cloud_url, new_main_url, sizeof(main_cloud_url)-1);
    ret = save_one_string_file(main_cloud_url_file_name, main_cloud_url, PROXY_MAIN_CLOUD_URL_FILE_NAME);

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

    strncpy(proxy_name, DEFAULT_PROXY_PROCESS_NAME, sizeof(proxy_name)-1);
    proxy_wd_to = DEFAULT_PROXY_WATCHDOG_TO_SEC;

    auth_token[0] = '\0';
    strncpy(auth_token_file_name, DEFAULT_AUTH_TOKEN_FILE_NAME, sizeof(auth_token_file_name)-1);
    device_id[0] = '\0';
    device_type = DEFAULT_PROXY_DEV_TYPE;
    cloud_url[0] = '\0';
    strncpy(main_cloud_url_file_name, DEFAULT_MAIN_CLOUD_URL_FILE_NAME, sizeof(main_cloud_url_file_name)-1);
    strncpy(main_cloud_url, DEFAULT_MAIN_CLOUD_URL, sizeof(main_cloud_url)-1);
    cloud_url_req_to_hrs = DEFAULT_CLOUD_URL_REQ_TO_HRS;

    queue_rec_amt = DEFAULT_QUEUE_RECORDS_AMT;

    agent_port = DEFAULT_AGENT_PORT;
    WUD_port = DEFAULT_WUD_PORT;

    strncpy(fw_version, DEFAULT_FW_VERSION_NUM, DEFAULT_FW_VERSION_SIZE-1);
    fw_ver_sending_to_hrs = DEFAULT_FW_VER_SENDING_TO_HRS;

    curlopt_ssl_verify_peer = DEFAULT_PROXY_CURLOPT_SSL_VERIFYPEER;
    curlopt_ca_info[0] = '\0';
    set_ssl_for_url_request = DEFAULT_SET_SSL_FOR_URL_REQUEST;

    strncpy(device_id_prefix, DEFAULT_PROXY_DEVICE_ID_PREFIX, sizeof(device_id_prefix));
    reboot_if_cloud_rejects = DEFAULT_REBOOT_IF_CLOUD_REJECTS;

    cloud_conn_timeout = LIB_HTTP_DEFAULT_CONNECT_TIMEOUT_SEC;
    cloud_post_attempts = LIB_HTTP_MAX_POST_RETRIES;
    long_get_keepalive_to = DEFAULT_KEEPALIVE_INTERVAL_SEC;
    long_get_to = DEFAULT_LONG_GET_TO_SEC;
    const char*tmp[] = DEFAULT_ALLOWED_DOMAINS;
    allowed_domains = pr_duplicate_ptr_list(allowed_domains, (char* const*)tmp);
    strncpy(proxy_listen_ip, DEFAULT_PROXY_LISTEN_IP, sizeof(proxy_listen_ip));
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
        PCS_ERR;
    }
    else {
        if(!strcmp(buf, PROXY_LL_DEBUG))  *llt_setting = LL_DEBUG;
        else if(!strcmp(buf, PROXY_LL_WARNING)) *llt_setting = LL_WARNING;
        else if(!strcmp(buf, PROXY_LL_INFO)) *llt_setting = LL_INFO;
        else if(!strcmp(buf, PROXY_LL_ERROR)) *llt_setting = LL_ERROR;
        else if(!strcmp(buf, PROXY_LL_TRACE_1)) *llt_setting = LL_TRACE_1;
        else if(!strcmp(buf, PROXY_LL_TRACE_2)) *llt_setting = LL_TRACE_2;
        else
            fprintf(stderr, "Setting %s = %s. Posssible values are %s, %s, %s or %s. Default will be used instead\n",
                   field_name, buf, PROXY_LL_DEBUG, PROXY_LL_INFO,  PROXY_LL_WARNING, PROXY_LL_ERROR
            );
    }
}
