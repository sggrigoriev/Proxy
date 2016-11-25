//
// Created by gsg on 29/10/16.
//
#include <assert.h>
#include <errno.h>
#include <zconf.h>
#include <pthread.h>
#include "string.h"
#include "stdio.h"
#include "stdlib.h"

#include "cJSON.h"

#include "libhttpcomm.h"
#include "pc_settings.h"
#include "pc_defaults.h"

/////////////////////////////////////////////////////////
//Config filelds names

#define PROXY_LOG_NAME          "LOG_NAME"
#define PROXY_LOG_REC_AMT       "LOG_REC_AMT"

#define PROXY_LOG_LEVEL         "LOG_LEVEL"
    #define PROXY_LL_DEBUG          "DEBUG"
    #define PROXY_LL_WARNING        "WARNING"
    #define PROXY_LL_INFO           "INFO"
    #define PROXY_LL_ERROR          "ERROR"

#define PROXY_SERTIFICAE_PATH   "SERTIFICAE_PATH"
#define PROXY_ACTIVATION_TOKEN  "ACTIVATION_TOKEN"
#define PROXY_DEVICE_ADDRESS    "DEVICE_ADDRESS"
#define PROXY_DEVICE_TYPE       "DEVICE_TYPE"
#define PROXY_CLOUD_URL         "CLOUD_URL"
#define PROXY_MAIN_CLOUD_URL    "MAIN_CLOUD_URL"

#define PROXY_UPLOAD_TO_SEC     "UPLOAD_TO_SEC"
#define PROXY_QUEUES_REC_AMT    "QUEUES_REC_AMT"
#define PROXY_AGENT_PORT        "AGENT_PORT"

////////////////////////////////////////////////////////
//Convig values
static char             log_name[PROXY_MAX_PATH];
static unsigned int     log_rec_amt;
static log_level_t      log_level;
static char             sertificate_path[PROXY_MAX_PATH];
static char             activation_token[PROXY_MAX_ACTIVATION_TOKEN_SIZE];
static char             device_address[PROXY_DEVICE_ID_SIZE];
unsigned int            device_type;
static char             cloud_url[PROXY_MAX_PATH];
static char             main_cloud_url[PROXY_MAX_PATH];
unsigned int            long_get_to;
unsigned int            queue_rec_amt;
unsigned int            agent_port;

static char conf_fname[PROXY_MAX_PATH];


/////////////////////////////////////////////////////////////
// Local functions
//
//Settings guard. Used in all "thread-protected" functions
static pthread_mutex_t local_mutex = PTHREAD_MUTEX_INITIALIZER;

//Indicates are defaults set 1 if set 0 if not
static int initiated = 0;

//Initiates config variables by defult values
static void initiate_defaults();

//Load config file "fname, parse it. Return pointer to cJSON object or NULL if error.
//NB! if config file is empty - return the empty cJSON object - "{}" parsing result
static cJSON* load_file(const char* fname);

//Update string item_name by value in cfg cJSON object
//If the item doesn't exist add the item to the object
static void json_str_update(const char* item_name, const char* value, cJSON* cfg);

//Update unsigned int item_name by value in cfg cJSON object
//If the item doesn't exist add the item to the object
static void json_uint_update(const char* item_name, unsigned int value, cJSON* cfg);

//Copy string value of field_name of cgf object into str_settings limited by max_size.
//Copy default value in case of absence of the field in the object or if the value size > max_size-1
static void getStrValue(cJSON* cfg, const char* field_name, char* str_setting, unsigned int max_size);

//Copy unsigned int value of field_name of cgf object into uint_setting.
//Copy default value in case of absence of the field in the object
static void getUintValue(cJSON* cfg, const char* field_name, unsigned int* uint_setting);

//Copy log_level_t value (see pu_logger.h) of field_name of cgf object into uint_setting.
//Copy default value in case of absence of the field in the object
static void getLLTValue(cJSON* cfg, const char* field_name, log_level_t* llt_setting);

//Safe the cfg object, translated to json ASCII text into text file fname. Return 1 if success and 0 othervize
static int saveToFile(const char* fname, cJSON* cfg);

//Update the correspondent setting old_value in memory plus updates the settings file: insert/update field name value by new_value
static int saveStrValue(const char* func_name, const char* field_name, const char *new_value, char* old_value, unsigned int max_size);
static int saveUintValue(const char* func_name, const char* field_name,  unsigned int new_value, unsigned int* old_value);

/////////////////////////////////////////////////////////////
#define PC_RET(a,b) return (!initiated)?a:b
////////////////////////////////////////////////////////////
//Set of "get" functions to make an access to settings for Presto modules

//Return LOG-file name
const char* pc_getLogFileName() {
    PC_RET(DEFAULT_LOG_NAME, log_name);
}

//Return max capacity of log file
unsigned int pc_getLogRecordsAmt() {
    PC_RET(DEFAULT_LOG_RECORDS_AMT, log_rec_amt);
}

//Return the min log level to be stored in LOG_FILE
//LL_DEBUG<LL_WARNING<LL_INFO<LL_ERROR
log_level_t pc_getLogVevel() {
    PC_RET(DEFAULT_LOG_LEVEL, log_level);
}

//Return the path to private sertificate file
//NB! used just during the interactive debug in sandbox
const char* pc_getSertificatePath() {
    PC_RET(DEFAULT_SERTIFICATE_PATH, sertificate_path);
}

//Return the timeout in seconds for "long get" made by Presto to listen the
//Cloud messages.
unsigned int pc_getLongGetTO() {
    PC_RET(DEFAULT_UPLOAD_TIMEOUT_SEC, long_get_to);
}

//Return the port# for communications with the Agent
unsigned int pc_getAgentPort() {
    PC_RET(DEFAULT_AGENT_PORT, agent_port);
}

//Return max amount of records kept in Presto queues
unsigned int pc_getQueuesRecAmt() {
    PC_RET(DEFAULT_QUEUE_RECORDS_AMT, queue_rec_amt);
}

//Return the Presto device type.(31 for now...)
unsigned int pc_getProxyDeviceType() {
    PC_RET(DEFAULT_PROXY_DEV_TYPE, queue_rec_amt);
}

/////////////////////////////////////////////////////////////
//Thread-protected functions
//
//Load configuration settings from cfg_file_name into the memory. All settings not mentioned in cfg_file_name
// got default values (see pc_defaults.h)
// return 1 if success, 0 if not
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
//Now load data
    getStrValue(cfg, PROXY_LOG_NAME, log_name, sizeof(log_name));
    getUintValue(cfg, PROXY_LOG_REC_AMT, &log_rec_amt);
    getLLTValue(cfg, PROXY_LOG_LEVEL, &log_level);
    getStrValue(cfg, PROXY_SERTIFICAE_PATH, sertificate_path, sizeof(sertificate_path));
    getStrValue(cfg, PROXY_ACTIVATION_TOKEN, activation_token, sizeof(activation_token));
    getStrValue(cfg, PROXY_DEVICE_ADDRESS, device_address, sizeof(device_address));
    getUintValue(cfg, PROXY_DEVICE_TYPE, &device_type);
    getStrValue(cfg, PROXY_CLOUD_URL, cloud_url, sizeof(cloud_url));
    getUintValue(cfg, PROXY_UPLOAD_TO_SEC, &long_get_to);
    getUintValue(cfg, PROXY_QUEUES_REC_AMT, &queue_rec_amt);
    getUintValue(cfg, PROXY_AGENT_PORT, &agent_port);

    cJSON_Delete(cfg);

    pthread_mutex_unlock(&local_mutex);
    return 1;
}

//Copy to the ret the clour uprl string
//return empty string if no value or the value > max_len
void pc_getCloudURL(char* ret, unsigned int max_len) { //return empty strinng if no value
    pthread_mutex_lock(&local_mutex);

    if(max_len < strlen(cloud_url)+1)
        strcpy(ret, "");
    else
        strcpy(ret, cloud_url);

    pthread_mutex_unlock(&local_mutex);
}

//Copy the the ret the default cloud contact point string
//Copy empty string if the value > max_len
void pc_getMainCloudURL(char* ret, unsigned int max_len) {
    pthread_mutex_lock(&local_mutex);

    if(max_len < strlen(main_cloud_url)+1)
        strcpy(ret, "");
    else
        strcpy(ret, main_cloud_url);

    pthread_mutex_unlock(&local_mutex);
}

//Copy to the ret the activation token string.
//Copy empty string if the value > max_len
void pc_getActivationToken(char* ret, unsigned int max_len) {
    pthread_mutex_lock(&local_mutex);

    if(max_len < strlen(activation_token)+1)
        strcpy(ret, "");
    else
        strcpy(ret, activation_token);

    pthread_mutex_unlock(&local_mutex);
}

//Copy to the ret the device address (deviceID) string.
//Copy empty string if the value > max_len
void pc_getDeviceAddress(char* ret, unsigned int max_len) {
    pthread_mutex_lock(&local_mutex);

    if(max_len < strlen(device_address)+1)
        strcpy(ret, "");
    else
        strcpy(ret, device_address);

    pthread_mutex_unlock(&local_mutex);
}

//Update/insert new activation token. Update the current value in memory as well
//Return 1 of success, return 0 if not
int pc_saveActivationToken(const char* new_at) {
    int ret;

    pthread_mutex_lock(&local_mutex);

    ret = saveStrValue("saveActivationToken", PROXY_ACTIVATION_TOKEN, new_at, activation_token, sizeof(activation_token));

    pthread_mutex_unlock(&local_mutex);
    return ret;
}

//Update/insert new device address (deviceID). Update the current value in memory as well
//Return 1 of success, return 0 if not
int pc_saveDeviceAddress(const char* new_da) {
    int ret;

    pthread_mutex_lock(&local_mutex);

    ret = saveStrValue("pc_saveDeviceAddress", PROXY_DEVICE_ADDRESS, new_da, device_address, sizeof(device_address));

    pthread_mutex_unlock(&local_mutex);
    return ret;
}

//Update/insert new default Cloud connection point. Update the current value in memory as well
//Return 1 of success, return 0 if not
int pc_saveMainCloudURL(const char* new_main_url) {
    int ret;

    pthread_mutex_lock(&local_mutex);

    ret = saveStrValue("pc_saveMainCloudURL", PROXY_MAIN_CLOUD_URL, new_main_url, main_cloud_url, sizeof(main_cloud_url));

    pthread_mutex_unlock(&local_mutex);
    return ret;
}

//Update configuration file name. NB! Think twice before the call this function notat the very beginning of all Presto!
//Return 1 of success, return 0 if not
int pc_saveCfgFileName(const char* new_file_name) {
    pthread_mutex_lock(&local_mutex);

    if(sizeof(conf_fname) < strlen(new_file_name)+1) return 0;

    strcpy(conf_fname, new_file_name);

    pthread_mutex_unlock(&local_mutex);
    return 1;
}

//Update/insert the port for communicating with the Agent. Update the current value in memory as well
//Return 1 of success, return 0 if not
int pc_saveAgentPort(unsigned int new_port) {
    int ret;
    pthread_mutex_lock(&local_mutex);

    ret = saveUintValue("pc_saveAgentPort", PROXY_AGENT_PORT, new_port, &agent_port);

    pthread_mutex_unlock(&local_mutex);
    return ret;
}

/////////////////////////////////////////////////////////////
//Activation-related stuff
//Used to calculate the level of activation:
//PC_FULL_ACTIVATION        - deviceID not found
//PC_ACTIVATION_KEY_NEEDED  - deviceID found, activation key not found
//PC_NO_NEED_ACTIVATION     - deviceID found, activation key found
//PC_TOTAL_MESS             - error - settings are not activated
//(see pc_settings.h)
pc_activation_type_t pc_calc_activation_type() { //Looks on deviceID and/or activation_token absence NB! what about the sertificate???
    char buf[PROXY_MAX_ACTIVATION_TOKEN_SIZE];
    if(!initiated) return PC_TOTAL_MESS;

    pc_getDeviceAddress(buf, sizeof(buf));
    if(!strlen(buf)) return PC_FULL_ACTIVATION;

    pc_getActivationToken(buf, sizeof(buf));
    if(!strlen(buf)) return PC_ACTIVATION_KEY_NEEDED;

    return PC_NO_NEED_ACTIVATION;
}

static void initiate_defaults() {
    initiated = 1;
    strcpy(log_name, DEFAULT_LOG_NAME);
    log_rec_amt = DEFAULT_LOG_RECORDS_AMT;
    log_level = DEFAULT_LOG_LEVEL;
    strcpy(sertificate_path, DEFAULT_SERTIFICATE_PATH);

    strcpy(activation_token, DEFAULT_ACTIVATION_TOKEN);         //NB! default is empty string!
    strcpy(device_address, DEFAULT_DEVICE_ADDRESS);             //NB! default is empty string!
    strcpy(cloud_url, DEFAULT_CLOUD_URL);                       //NB! default is empty string!

    long_get_to = DEFAULT_UPLOAD_TIMEOUT_SEC;
    queue_rec_amt = DEFAULT_QUEUE_RECORDS_AMT;
    agent_port = DEFAULT_AGENT_PORT;
}

//getStrValue() - get string value
//cfg           - poiner to cJSON object containgng configuration
//field_name    - JSON fileld name
//str_setting   - returned value of field_name
//max_size      - str_setting capacity
static void getStrValue(cJSON* cfg, const char* field_name, char* str_setting, unsigned int max_size) {
    cJSON* obj;
    if(obj = cJSON_GetObjectItem(cfg, field_name), obj == NULL)
        fprintf(stderr, "Setting %s is not found. Default will be used instead.\n", field_name);
    else if(strlen(obj->valuestring) > max_size-1) {
        fprintf(stderr, "Setting %s value > than max size: %d against %d. Default will be used instead.\n", field_name, strlen(field_name), max_size);
    }
    else {
        strcpy(str_setting, obj->valuestring);
    }
}

//getUintValue()    - get uint value
//cfg           - poiner to cJSON object containgng configuration
//field_name    - JSON fileld name
//uint_setting   - returned value of field_name
static void getUintValue(cJSON* cfg, const char* field_name, unsigned int* uint_setting) {
    cJSON* obj;
    if(obj = cJSON_GetObjectItem(cfg, field_name), obj == NULL)
        fprintf(stderr, "Setting %s is not found. Default will be used instead.\n", field_name);
    else
        *uint_setting = (unsigned int)obj->valueint;
}

//getLLTValue()    - get log_level_t value
//cfg           - poiner to cJSON object containgng configuration
//field_name    - JSON fileld name
//llt_setting   - returned value of field_name
static void getLLTValue(cJSON* cfg, const char* field_name, log_level_t* llt_setting) {
    cJSON* obj;
    if(obj = cJSON_GetObjectItem(cfg, field_name), obj == NULL)
        fprintf(stderr, "Setting %s is not found. Default will be used instead.\n", field_name);
    else {
        char* res = obj->valuestring;
        if(!strcmp(res, PROXY_LL_DEBUG))  *llt_setting = LL_DEBUG;
        else if(!strcmp(res, PROXY_LL_WARNING)) *llt_setting = LL_WARNING;
        else if(!strcmp(res, PROXY_LL_INFO)) *llt_setting = LL_INFO;
        else if(!strcmp(res, PROXY_LL_ERROR)) *llt_setting = LL_ERROR;
        else
            fprintf(stderr, "Setting %s = %s. Posssible values are %s, %s, %s or %s. Default will be used instead\n",
                   field_name, res, PROXY_LL_DEBUG, PROXY_LL_INFO,  PROXY_LL_WARNING, PROXY_LL_ERROR
            );
    }
}

//load_file()    - load configuration file into memory
//fname           - file name
//Return NULL if no file or bad JSON, or cJSON object
static cJSON* load_file(const char* fname) {
    FILE *f;
    char buffer[100];
    char* cfg = NULL;
    cJSON* ret = NULL;

    if(f = fopen(fname, "r"), f == NULL) {
        fprintf(stderr, "Config file %s open error %d %s\n", fname, errno, strerror(errno));
        return 0;
    }
    int ptr = 0;
    while (fgets(buffer, sizeof(buffer), f)) {
        cfg = realloc(cfg, strlen(buffer)+ptr);
        memcpy(cfg+ptr, buffer, strlen(buffer));
        ptr += strlen(buffer);
    }
    cfg = realloc(cfg, ptr+1);
    cfg[ptr] = '\0';
    fclose(f);

    if(!strlen(cfg)) {
        fprintf(stderr, "Configuration file %s is empty\n", fname);
        return cJSON_Parse("{}");
    }

    ret = cJSON_Parse(cfg);
    if(ret == NULL) {
        fprintf(stderr, "Error parsing the following:%s\n", cfg);
    }
    free(cfg);
    return ret;
}

//json_str_update() - update field of string type
//item_name     - fileld name
//value         - new value
//cfg           - pointer to cJSON object
static void json_str_update(const char* item_name, const char* value, cJSON* cfg) {
    cJSON* at;

    if(at = cJSON_GetObjectItem(cfg, item_name), at == NULL) {         //Add item
        fprintf(stderr, "%s item is not found in configuretion file. Added.\n", item_name);
        cJSON_AddItemToObject(cfg, item_name, cJSON_CreateString(value));
    }
    else {
        free(at->valuestring);                                                      //UPdate Item value
        at->valuestring = strdup(value);
    }
}

//json_uint_update() - update field of uint type
//item_name     - fileld name
//value         - new value
//cfg           - pointer to cJSON object
static void json_uint_update(const char* item_name, unsigned int value, cJSON* cfg) {
    cJSON* at;

    if(at = cJSON_GetObjectItem(cfg, item_name), at == NULL) {         //Add item
        fprintf(stderr, "%s item is not found in configuretion file. Added.\n", item_name);
        cJSON_AddItemToObject(cfg, item_name, cJSON_CreateNumber(value));
    }
    else {
        at->valueint = value;        //UPdate Item value
    }
}

//saveToFile() - save cfg to the fname
//fname     - file name
//cfg       - pointer to cJSON object
static int saveToFile(const char* fname, cJSON* cfg) { //Returns 0 if bad
    FILE *f;
    int ret = 0;
    char temp_name[L_tmpnam];

    strcpy(temp_name, "CONFIG_TEMPXXXXXX");

    close(mkstemp(temp_name));
    if(f = fopen(temp_name, "w"), f == NULL) {
        fprintf(stderr, "Saving config file: Unable to open temp file. Save Failed %d %s\n", errno, strerror(errno));
    }
    else {
        fprintf(f,"%s\n", cJSON_Print(cfg));
        fclose(f);
        if(unlink(fname)) {
            fprintf(stderr, "Can\'t delete the file %s. Changed config file saved in %s. %d %s\n", fname, temp_name, errno, strerror(errno));
        }
        else if(rename(temp_name, fname)) {
            fprintf(stderr, "Can\'t rename the file %s to %s. Changed config file saved in %s. %d %s\n", temp_name, fname, temp_name, errno, strerror(errno));
        }
        else
            ret = 1;
    }
    cJSON_Delete(cfg);
    return ret;
 }

//saveStrValue() - save string type value to the fname
//fname     - file name
//field_name    - filend name
//new_value     - new value
//old_value     - poiner to the setting in memory
//max_size      - old_value capacity
static int saveStrValue(const char* func_name, const char* field_name, const char *new_value, char* old_value, unsigned int max_size) {
    if(!initiated) {
        fprintf(stderr, "%s(): Failed. Configuration utulity is not activated!\n", func_name);  \
        return 0;
    }
    if(strlen(new_value)+1 > max_size) {
        fprintf(stderr, "%s(): new value %s is too big: %d against %d\n", func_name, new_value, strlen(new_value), max_size);
        return 0;
    }

    strcpy(old_value, new_value);     // Save into loaded settings

    cJSON* cfg = load_file(conf_fname);
    if(cfg == NULL) return 0;

    json_str_update(field_name, new_value, cfg);
    return saveToFile(conf_fname, cfg);
}

//saveUintValue() - save uint type value to the fname
//fname     - file name
//field_name    - filend name
//new_value     - new value
//old_value     - poiner to the setting in memory
static int saveUintValue(const char* func_name, const char* field_name,  unsigned int new_value, unsigned int* old_value) {
    if(!initiated) {
        fprintf(stderr, "%s(): Failed. Configuration utulity is not activated!\n", func_name);  \
        return 0;
    }

    *old_value = new_value;     // Save into loaded settings

    cJSON* cfg = load_file(conf_fname);
    if(cfg == NULL) return 0;

    json_uint_update(field_name, new_value, cfg);
    return saveToFile(conf_fname, cfg);
}