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
    Created by gsg on 29/10/16.

    contains all proxy settings and functions to acess it
*/

#ifndef PRESTO_PC_SETTINGS_H
#define PRESTO_PC_SETTINGS_H

#include <pu_logger.h>


/*
    Set of "get" functions to make an access to settings for Presto modules
*/
const char* pc_getProxyName();              /* Return process name */

const char*     pc_getLogFileName();        /* Return LOG-file name */
size_t          pc_getLogRecordsAmt();      /* Return max capacity of log file */
log_level_t     pc_getLogVevel();           /*  Return the min log level to be stored in LOG_FILE */

size_t          pc_getQueuesRecAmt();       /* Return max amount of records kept in Presto queues */

unsigned int    pc_getProxyDeviceType();    /* Obsolete */
unsigned int    pc_getProxyWDTO();          /* timeout for watchdog sending */
unsigned int    pc_getCloudURLTOHrs();      /* timeout for new contact URL request */
unsigned int    pc_getFWVerSendToHrs();     /* timeout for fw version sending to the cloud */

unsigned int    pc_getWUDPort();            /* Return WUD communication port */

unsigned int    pc_getCloudConnTimeout();   /* Return connection to cloud timeout for POST */
unsigned int    pc_getCloudPostAttempts();  /* Times to repeat post attempts to cloud */
unsigned int    pc_getLongGetKeepaliveTO(); /* Returns keepalive intervals during the long get TO */
unsigned int    pc_getLongGetTO();          /* Return the timeout in seconds for "long get" made by Presto to listen the Cloud messsage */

/*  For Agent emulator */
unsigned int    pc_getAgentPort();         /* Return the port# for communications with the Agent */

/**************************************************************************************************************************
    Thread-protected functions
*/
/* Initiate the configuration service. Load data from configuration port; Initiates default values
 *  Return 1 if Ok, 0 if not
 */
int pc_load_config(const char* cfg_file_name);

void pc_getCloudURL(char* ret, size_t size);        /* return empty string if no value or the value > max_len */
void pc_getMainCloudURL(char* ret, size_t size);    /* return empty string if no value or the value > max_len */
void pc_getAuthToken(char* ret, size_t size);       /* return empty string if no value or the value > max_len */
void pc_getProxyDeviceID(char* ret, size_t size);   /* same... */

/* Update the current value in memory & in file! */
int pc_saveAuthToken(const char* new_at);           /* Return 1 of success, return 0 if not */

int pc_saveProxyDeviceID(const char* new_da);       /* Return 1 of success, return 0 if not */
int pc_saveMainCloudURL(const char* new_main_url);  /* Return 1 of success, return 0 if not */

/* Update the current value in memory only! */
int pc_saveCloudURL(const char* new_url);           /* Return 1 of success, return 0 if not */

int pc_saveCfgFileName(const char* new_file_name);  /* Return 1 of success, return 0 if not */
int pc_saveAgentPort(unsigned int new_port);        /* Return 1 of success, return 0 if not */

void pc_readFWVersion();                                /* Reads the version from DEFAULT_FW_VERSION_FILE */
void pc_getFWVersion(char* fw_version, size_t size);    /* Get the firmvare version or default value */

/* Activation-related stuff */
int pc_existsProxyDeviceID();                /* Return 1 if defice id exists and sohuld not be generated. Will be removed once upon a time... */

/* Used in Contact URL request */
int pc_setSSLForCloudURLRequest();          /* Return 1 if "SET_SSL_FOR_URL_REQUEST" set to 1 or not found. 0 otherwize */

int pc_getCurloptSSPVerifyPeer();
const char* pc_getCurloptCAInfo();

const char* pc_getProxyDeviceIDPrefix();
int pc_rebootIfCloudRejects();

#endif /*PRESTO_PC_SETTINGS_H*/
