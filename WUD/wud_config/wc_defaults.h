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

    All defaults for WUD process are here
*/

#ifndef PRESTO_WC_DEFAULTS_H
#define PRESTO_WC_DEFAULTS_H

#include "lib_http.h"

#define WC_MAX_PATH                         LIB_HTTP_MAX_URL_SIZE       /* Max path length */
#define WC_MAX_PROCESS_NAME_LEN             30                          /* Max process name allowed (used for childs management */
#define WC_MAX_MSG_LEN                      LIB_HTTP_MAX_MSG_SIZE       /* Max incoming message lenth allowed */

#define WD_DEFAULT_CFG_FILE_NAME            "./wud.conf"                /* Hardcoded configuration file name */
#define WD_DEFAULT_REBOOT_BY_REQUEST        1

/* Common defaults */
#define WC_DEFAULT_LOG_NAME                 "./WUD_LOG"                 /* WUD log file name. Configured */
#define WC_DEFAULT_LOG_RECORDS_AMT          5000                        /* Max records amount in log file. Configured */
#define WC_DEFAULT_LOG_LEVEL                LL_DEBUG                    /* Logging level. Configured */

#define WC_DEFAULT_QUEUES_REC_AMT           1024                        /* Max records in each WUD queue. Configured */

#ifdef WUD_ON_HOST                                          /* Defined in WUD CMakeLists - means WUD on Ubuntu machine */
    #define WC_DEFAULT_PID_DIRECTORY        "/var/run/"                 /* Directory to save own PID in file "wud.pid" */
#else
    #define WC_DEFAULT_PID_DIRECTORY        "/tmp/"
#endif

#define WC_DEFAULT_WUD_NAME                 "wud"                       /* Process name. Used to construct file name with own pid */
#define WC_DEFAULT_PIDF_EXTENCION           "pid"                       /* File extention for the file with own pid */
#define WC_PROC_DIR                         "/proc/"                    /* The source directory to find the own PID */
#define WC_DEFAULT_FWU_FILE_NAME                    "presto_mt7688.tgz"         /* File name for ready for install firmware */

#define WUD_DEFAULT_WORKING_DIRECTORY       "./"                        /* WUD working directory. Configurable */
#define WUD_DEFAULT_COMM_PORT               8887                        /* WUD communication port. Configurable */
#define WUD_DEFAULT_LISTEN_IP               "0.0.0.0"
#define WUD_DEFAULT_CHILDREN_SHUTDOWN_TO_SEC    120                     /* Timeout in seconds to wait until the force shutdown of child processes- Proxy & Agent. Configurable */

#define WUD_DEFAULT_FW_DOWNLOAD_FOLDER      "./download"                /* Directory to download firmware file. Configurable */
#define WUD_DEFAULT_FW_UPGRADE_FOLDER       "./upgrade"                 /* Directory to put firmware file for further installation. Configuravble */
#define WUD_DEFAULT_FW_COPY_EXT             "part"                      /* Extention for the firmware file during the copy from download directory */

#define WUD_DEFAULT_AGENT_PROCESS_NAME      "Agent"                     /* Agent process process name. Configurable */
#define WUD_DEFAULT_AGENT_BINARY_NAME       "agent"                     /* Agent executable name. Configurable */

#define WUD_DEFAULT_AGENT_WORKING_DIRECTORY  "./agent_wd"               /* Agent working directory. Configurable */
#define WUD_DEFAULT_AGENT_WD_TIMEOUT_SEC      100                       /* Timeout in seconds for Agent's watchdogs. Configurable */

#define WUD_DEFAULT_PROXY_PROCESS_NAME       "Proxy"                    /* Proxy process process name. Configurable */
#define WUD_DEFAULT_PROXY_BINARY_NAME        "proxy"                    /* Proxy executable name. Configurable */

#define WUD_DEFAULT_PROXY_WORKING_DIRECTORY  "./proxy_wd"               /* Proxy working directory. Configurable */
#define WUD_DEFAULT_PROXY_WD_TIMEOUT_SEC      100                       /* Timeout in seconds for Proxy's watchdogs. Configurable */

#define WUD_DEFAULT_MONITORING_TO_SEC         3600                      /* WUD monitoring period in seconds. Configurable */
#define WUD_DEFAULT_SERVER_WRITE_TO_SEC       1                         /* Timeout for WUD POSTs in seconds. 0 means no timeout. */
#define WUD_DEFAILT_DELAY_BEFORE_START_SEC    1                         /* WUD delay on startup in seconds. Configurable */

#define WUD_DEFAULT_CURLOPT_SSL_VERIFYPEER    1

#define WUD_DEFAULT_EXIT_ON_FW_UPGRADE          2                       /* exit rc if no reboot */
#define WUD_DEFAULT_EXIT_ON_CHILD_HANGS         1                       /* exit rc if no reboot */
#define WUD_DEFAULT_EXIT_ON_ERROR               3
#define WUD_DEFAULT_EXIT_JUST_BECAUSE           0

#endif /* PRESTO_WC_DEFAULTS_H */
