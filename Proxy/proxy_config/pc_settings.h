//
// Created by gsg on 29/10/16.
//
// contains all proxy settings and functions to acess it
//

#ifndef PRESTO_PC_SETTINGS_H
#define PRESTO_PC_SETTINGS_H

#include <pu_logger.h>


//////////////////////////////////////////////////////////////////////////
//Set of "get" functions to make an access to settings for Presto modules

const char* pc_getProxyName();      //Return process name

const char* pc_getLogFileName();        //Return LOG-file name
size_t pc_getLogRecordsAmt();     //Return max capacity of log file
log_level_t pc_getLogVevel();           //Return the min log level to be stored in LOG_FILE
const char* pc_getSertificatePath();    //Return the path to private sertificate file
unsigned int pc_getLongGetTO();         //Return the timeout in seconds for "long get" made by Presto to listen the Cloud messsage
unsigned int pc_getAgentPort();         //Return the port# for communications with the Agent
size_t pc_getQueuesRecAmt();      //Return max amount of records kept in Presto queues
unsigned int pc_getProxyDeviceType();   //Used in eui64 - to make the deviceID string
unsigned int pc_getWUDPort();

//////////////////////////////////////////////////////////////////////////
//Thread-protected functions

int pc_load_config(const char* cfg_file_name);                  // return 1 if success, 0 if not

void pc_getCloudURL(char* ret, size_t max_len);           //return empty string if no value or the value > max_len
void pc_getMainCloudURL(char* ret, size_t max_len);       //return empty string if no value or the value > max_len
void pc_getActivationToken(char* ret, size_t max_len);    //return empty string if no value or the value > max_len
void pc_getDeviceAddress(char* ret, size_t max_len);

int pc_saveActivationToken(const char* new_at);     //Return 1 of success, return 0 if not
int pc_saveDeviceAddress(const char* new_da);       //Return 1 of success, return 0 if not
int pc_saveMainCloudURL(const char* new_main_url);  //Return 1 of success, return 0 if not
int pc_saveCfgFileName(const char* new_file_name);  //Return 1 of success, return 0 if not
int pc_saveAgentPort(unsigned int new_port);        //Return 1 of success, return 0 if not

//Activation-related stuff
//
typedef enum {PC_FULL_ACTIVATION, PC_ACTIVATION_KEY_NEEDED, PC_NO_NEED_ACTIVATION, PC_TOTAL_MESS} pc_activation_type_t;

pc_activation_type_t pc_calc_activation_type(); //Looks on deviceID and/or activation_token absence

#endif //PRESTO_PC_SETTINGS_H
