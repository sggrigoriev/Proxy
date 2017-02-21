//
// Created by gsg on 29/11/16.
//

#ifndef PRESTO_WC_SETTINGS_H
#define PRESTO_WC_SETTINGS_H

#include "pu_logger.h"


const char* wc_getLogFileName();
unsigned int wc_getLogRecordsAmt();
log_level_t wc_getLogVevel();

unsigned int wc_getQueuesRecAmt();

const char* wc_getWorkingDir();
unsigned int wc_getWUDPort();

unsigned int wc_getChildrenShutdownTO();

const char* wc_getFWDownloadFolder();
const char* wc_getFWUpgradeFolder();

const char* wc_getAgentProcessName();

const char* wc_getAgentBinaryName();
char* const* wc_getAgentRunParameters();

const char* wc_getAgentWorkingDirectory();
unsigned int wc_getAgentWDTimeoutSec();

const char* wc_getProxyProcessName();
const char* wc_getProxyBinaryName();
char* const* wc_getProxyRunParameters();

const char* wc_getProxyWorkingDirectory();
unsigned int wc_getProxyWDTimeoutSec();

unsigned int wc_getWUDMonitoringTO();

//thread-protected functions
int wc_load_config(const char* cfg_file_name);

void wc_getDeviceID(char* di, size_t size);
void wc_setDeviceID(const char* di);
void wc_getURL(char* urk, size_t size);
void wc_setURL(const char* url);
void wc_getAuthToken(char* at, size_t size);
void wc_setAuthToken(const char* at);
void wc_getFWVersion(char* ver, size_t size);
void wc_setFWVersion(const char* ver);
#endif //PRESTO_WC_SETTINGS_H
