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
    Created by gsg on 20/11/16.
-
     Contains defaults for Proxy process
*/

#ifndef PRESTO_PC_DEFAULTS_H
#define PRESTO_PC_DEFAULTS_H

#include "lib_http.h"
#include "pr_commands.h"
#include "eui64.h"

#define DEFAULT_CFG_FILE_NAME             "./proxyJSON.conf"/* Default Proxy configuration file */
#define DEFAULT_AUTH_TOKEN_FILE_NAME      "./auth_token"    /* File name for authentication token. Configurable */
#define DEFAULT_CLOUD_URL_FLIE_NAME       "./cloud_url"     /* File name with main contact URL */


/* Common defaults */
#define DEFAULT_LOG_NAME        "./COMM_LOG"        /* Proxy log file name. Configurable */
#define DEFAULT_LOG_RECORDS_AMT 5000                /* Max records amount in the log. Configurable */
#define DEFAULT_LOG_LEVEL       LL_ERROR            /* Log level. Configurable. */

/* Connectivity defaults */

/* Global constants for Proxy communications with Server */

#define PROXY_MAX_HTTP_RETRIES                                  LIBB_HTTP_MAX_POST_RETRIES  /* Max connection attempts before the reconnect */
#define PROXY_MAX_MSG_LEN                                       LIB_HTTP_MAX_MSG_SIZE       /* Max message length to be accepted by Proxy */
#define PROXY_NUM_SERVER_CONNECTIONS_BEFORE_SYSLOG_NOTIFICATION 20                          /* Not used. Inherited from past Presto */
#define PROXY_MAX_PUSHES_ON_RECEIVED_COMMAND                    2                           /* Same */
#define PROXY_HEADER_PASSWORD_LEN                               64                          /* Same */
#define PROXY_HEADER_KEY_LEN                                    256                         /* Same */

#define PROXY_URL_SIZE                  LIB_HTTP_MAX_URL_SIZE                   /* Max size of string with URL */
#define PROXY_MAX_HTTP_SEND_MESSAGE_LEN LIB_HTTP_MAX_MSG_SIZE
#define PROXY_MAX_ACTIVATION_TOKEN_SIZE LIB_HTTP_AUTHENTICATION_STRING_SIZE     /* Max len of auth token */
#define PROXY_DEVICE_ID_SIZE            LIB_HTTP_DEVICE_ID_SIZE                 /* Max len of defice id */
#define PROXY_MAX_PATH                  LIB_HTTP_MAX_URL_SIZE                   /* Max len of path allowed */
#define PROXY_MAX_PROC_NAME             PR_MAX_PROC_NAME_SIZE                   /* Max len of child process name allowed */

#define DEFAULT_PROXY_PROCESS_NAME  "Proxy"                         /* Proxy process name. Configured */
#define DEFAULT_PROXY_WATCHDOG_TO_SEC   600                         /* Proxy watchdog preiod in seconds. Configured */
#define DEFAULT_AGENT_PROCESS_NAME  "Agent"                         /* Agent process name (for emilator). Configured */

#define DEFAULT_MAIN_CLOUD_URL_FILE_NAME "./cloud_url"         /* Default name for thr file with main cloud URL name. Configured */
#define DEFAULT_MAIN_CLOUD_URL      "https://app.presencepro.com"   /* Default main cloud URL. Configured */
#define DEFAULT_SET_SSL_FOR_URL_REQUEST 1                           /* Ask for contact URL from main URL with ssl=true */

#define DEFAULT_CLOUD_URL_REQ_TO_HRS    24                          /* Preiod of cloud conection URL request in hours. Configured */
#define DEFAULT_FW_VER_SENDING_TO_HRS   24                          /* Period of firmware version sending to the cloud in hours. Configured */

#define DEFAULT_LONG_GET_TO_SEC         120         /* Log get timeout. Configfured */
#define DEFAULT_KEEPALIVE_INTERVAL_SEC  20      /* Keep alive TCP signals intervals during the long GET. Configured */
#define DEFAULT_PROXY_DEV_TYPE      31          /* Obsolete. The Agent sends it by itself Anyway, configured */


#define DEFAULT_HTTP_CONNECT_TIMEOUT_SEC    HTTPCOMM_DEFAULT_CONNECT_TIMEOUT_SEC    /* HTTP connection timeout in seconds */
#define DEFAULT_HTTP_TRANSFER_TIMEOUT_SEC   HTTPCOMM_DEFAULT_TRANSFER_TIMEOUT_SEC   /* Don't remember... Something very important. Use grep - it helps... */

/* Global constants for Proxy inner communications and communications with Agent */
#define DEFAULT_AGENT_PORT              8888                    /* This port is used for emulator only! It is busy on MT platform! Configured */
#define DEFAULT_BINDING_ATTEMPTS        PT_BINDING_ATTEMPTS     /* Binging attempts for TCP port */
#define DEFAULT_SOCK_SELECT_TO_SEC      PT_SOCK_SELECT_TO_SEC   /* Timeout for select function in seconds */

#define DEFAULT_TCP_ASSEMBLING_BUF_SIZE 24576                   /* Assembling bufer size for incoming messages in TCP */
#define DEFAULT_QUEUE_RECORDS_AMT       1024                    /* MAx elements in queue. Same for all Proxy queues. Configured */

#define DEFAULT_WUD_PORT                    8887                /* Port to communicate with WUD. Configured */

#ifdef PROXY_ON_HOST                /* NB! this is cmake define! Works if Proxy built for Ubuntu */
    #define DEFAULT_FW_VERSION_FILE         "./firmware_release"            /* file with current firmware version */
#else
    #define DEFAULT_FW_VERSION_FILE         "/root/presto/firmware_release"
#endif

#define DEFAULT_FW_VERSION_SIZE             LIB_HTTP_FW_VERSION_SIZE        /* Max size of version */
#define DEFAULT_FW_VERSION_NUM              "undefined firmware version"    /* Default version if the file is not found */

#define DEFAULT_PROXY_CURLOPT_SSL_VERIFYPEER    1

#define DEFAULT_PROXY_DEVICE_ID_PREFIX          "aioxGW-"
#define DEFAULT_REBOOT_IF_CLOUD_REJECTS         0

#endif /*PRESTO_PC_DEFAULTS_H */
