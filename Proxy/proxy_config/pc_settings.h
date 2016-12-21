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

const char*     pc_getLogFileName();        //Return LOG-file name
size_t          pc_getLogRecordsAmt();     //Return max capacity of log file
log_level_t     pc_getLogVevel();           //Return the min log level to be stored in LOG_FILE
unsigned int    pc_getLongGetTO();         //Return the timeout in seconds for "long get" made by Presto to listen the Cloud messsage
unsigned int    pc_getAgentPort();         //Return the port# for communications with the Agent
size_t          pc_getQueuesRecAmt();      //Return max amount of records kept in Presto queues
unsigned int    pc_getProxyDeviceType();   //Used in eui64 - to make the deviceID string
unsigned int    pc_getWUDPort();            //
unsigned int    pc_getAgentWDTO();          // timeout for watchdog sending

//////////////////////////////////////////////////////////////////////////
//Thread-protected functions

int pc_load_config(const char* cfg_file_name);                  // return 1 if success, 0 if not

void pc_getCloudURL(char* ret, size_t size);           //return empty string if no value or the value > max_len
void pc_getMainCloudURL(char* ret, size_t size);       //return empty string if no value or the value > max_len
void pc_getActivationToken(char* ret, size_t size);    //return empty string if no value or the value > max_len
void pc_getProxyDeviceID(char* ret, size_t size);

int pc_saveActivationToken(const char* new_at);     //Return 1 of success, return 0 if not
int pc_saveProxyDeviceID(const char* new_da);       //Return 1 of success, return 0 if not
int pc_saveMainCloudURL(const char* new_main_url);
int pc_saveCloudURL(const char* new_url);           //Return 1 of success, return 0 if not
int pc_saveCfgFileName(const char* new_file_name);  //Return 1 of success, return 0 if not
int pc_saveAgentPort(unsigned int new_port);        //Return 1 of success, return 0 if not

//Activation-related stuff
//
int pc_existsCloudURL();
int pc_existsActivationToken();
int ps_existsProxyDeviceID();

#endif //PRESTO_PC_SETTINGS_H
