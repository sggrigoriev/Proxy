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
    Interfaces for WUD configuration settings load and access
*/

#ifndef PRESTO_WC_SETTINGS_H
#define PRESTO_WC_SETTINGS_H

#include "pu_logger.h"


const char* wc_getLogFileName();            /* Return the WUD log file path&name */
unsigned int wc_getLogRecordsAmt();         /* Return max amount of records in log file allowed */
log_level_t wc_getLogVevel();               /* Return the log level for WUD logging */

unsigned int wc_getQueuesRecAmt();          /* Return the max records amount in each queue used in QUD */
unsigned int wc_getRebootByRequest();

const char* wc_getWorkingDir();             /* Return WUD working directory */
unsigned int wc_getWUDPort();               /* Return WUD communication port (read from Proxy & Agent) */
const char* wc_getWUDListenIP();            /* Return WUD IP address to listen */

unsigned int wc_getChildrenShutdownTO();    /* Return the timeout in seconds before the force chidren's sutdown */

const char* wc_getFWDownloadFolder();       /* Return the folder to store the downloaded firmware file */
const char* wc_getFWUpgradeFolder();        /* Return the folder to store ready for installation firmware file */
const char* wc_getFWUpgradeFileName();      /* Return the file name for firmware file */

const char* wc_getAgentProcessName();       /* Return the Agent process' name */

const char* wc_getAgentBinaryName();        /* Return the Agent executable name */
char* const* wc_getAgentRunParameters();    /* Return Agent's process start parameters NULL-terminated list */

const char* wc_getAgentWorkingDirectory();  /* Return the Agent working directory */
unsigned int wc_getAgentWDTimeoutSec();     /* Return the timeout in seconds to check Agent's watchdogs arrival */

const char* wc_getProxyProcessName();       /* Return the Proxy process' name */
const char* wc_getProxyBinaryName();        /* Return the Proxy executable name */
char* const* wc_getProxyRunParameters();    /* Return Proxy's process start parameters NULL-terminated list */

const char* wc_getProxyWorkingDirectory();  /* Return the Proxy working directory */
unsigned int wc_getProxyWDTimeoutSec();     /* Return the timeout in seconds to check Proxy's watchdogs arrival */

unsigned int wc_getWUDMonitoringTO();       /* Return the WUD monitoring process period in seconds */
unsigned int wc_getWUDDelay();              /* Return the pause in seconds before the WUD start */

const char* wc_getCurloptCAInfo();
int wc_getCurloptSSLVerifyPeer();

unsigned int    wc_getCloudConnTimeout();   /* Return connection to cloud timeout for POST */
unsigned int    wc_getCloudPostAttempts();  /* Times to repeat post attempts to cloud */
unsigned int    wc_getCloudFileGetConnTimeout();    /* Connetction TO fot File GET */
unsigned int    wc_getKeepAliveInterval();   /* Period in seconds between keep idle messages during the file get */



/* Thread-protected functions */
/**
 * Load WUD configuration file; Save values in memory.
 *
 * @param cfg_file_name - WUD configuration file name
 * @return  - 1 if OK, 0 if error
 */
int wc_load_config(const char* cfg_file_name);

/**
 * Get the gateway device id
 *
 * @param di    - buffer for device id received
 * @param size  - buffer size
 */
void wc_getDeviceID(char* di, size_t size);

/**
 * Store the gateway device id in memory
 *
 * @param di    - the device id to be stored
 */
void wc_setDeviceID(const char* di);

/**
 * Get cloud contact URL
 *
 * @param url   - buffer for URL received
 * @param size  - buffer size
 */
void wc_getURL(char* url, size_t size);

/**
 * Save the clud contact URL in memory
 *
 * @param url   - URL to be saved
 */
void wc_setURL(const char* url);

/**
 * Get the gateway authentication token
 *
 * @param at    - buffer to save the token
 * @param size  - buffer size
 */
void wc_getAuthToken(char* at, size_t size);

/**
 * Save the gateway auth token in memory
 *
 * @param at    - auth token to be saved
 */
void wc_setAuthToken(const char* at);

/**
 * Get the gateway firmware version
 *
 * @param ver   - buffer to save firmware version
 * @param size  - buffer size
 */
void wc_getFWVersion(char* ver, size_t size);

/**
 * Save the gateway firmware verion in memory
 * @param ver   - firmware version to save
 */
void wc_setFWVersion(const char* ver);

#endif /* PRESTO_WC_SETTINGS_H */
