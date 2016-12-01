//
// Created by gsg on 20/11/16.
//
// Contains default settings for all Presto parts

#ifndef PRESTO_PC_DEFAULTS_H
#define PRESTO_PC_DEFAULTS_H

#include "pt_tcp_utl.h"
#include "libhttpcomm.h"
#include "eui64.h"

#define DEFAULT_CFG_FILE_NAME   "/media/sf_GWDrive/Presto_new/build/proxyJSON.conf"

//Common defaults
#define DEFAULT_LOG_NAME        "/media/sf_GWDrive/Presto_new/build/COMM_LOG"
#define DEFAULT_LOG_RECORDS_AMT 5000
#define DEFAULT_LOG_LEVEL       LL_DEBUG

//Connectivity defaults
////////////////////////////////////////////
// Global constants for Proxy communications with Server

#define PROXY_MAX_HTTP_RETRIES                                  3
#define PROXY_MAX_MSG_LEN                                       8192
#define PROXY_NUM_SERVER_CONNECTIONS_BEFORE_SYSLOG_NOTIFICATION 20
#define PROXY_MAX_PUSHES_ON_RECEIVED_COMMAND                    2
#define PROXY_HEADER_PASSWORD_LEN                               64
#define PROXY_HEADER_KEY_LEN                                    256

#define PROXY_URL_SIZE                  256
#define PROXY_MAX_HTTP_SEND_MESSAGE_LEN 32768U
#define PROXY_MAX_ACTIVATION_TOKEN_SIZE 128
#define PROXY_DEVICE_ID_SIZE            EUI64_STRING_SIZE
#define PROXY_MAX_PATH                 4097

#define DEFAULT_SERTIFICATE_PATH    "/media/sf_GWDrive/SSHKey/GSG_Key.pub"
#define DEFAULT_MAIN_CLOUD_URL      ""
#define DEFAULT_CLOUD_URL           ""
#define DEFAULT_UPLOAD_TIMEOUT_SEC  120
#define DEFAULT_ACTIVATION_TOKEN    ""
#define DEFAULT_DEVICE_ADDRESS      ""
#define DEFAULT_PROXY_DEV_TYPE      31

#define DEFAULT_HTTP_CONNECT_TIMEOUT_SEC    HTTPCOMM_DEFAULT_CONNECT_TIMEOUT_SEC
#define DEFAULT_HTTP_TRANSFER_TIMEOUT_SEC   HTTPCOMM_DEFAULT_TRANSFER_TIMEOUT_SEC

//Global constants for Proxy inner communications and communications with Agent
#define DEFAULT_AGENT_PORT              8888
#define DEFAULT_BINDING_ATTEMPTS        PT_BINDING_ATTEMPTS
#define DEFAULT_SOCK_SELECT_TO_SEC      PT_SOCK_SELECT_TO_SEC

#define DEFAULT_TCP_ASSEMBLING_BUF_SIZE 24576
#define DEFAULT_QUEUE_RECORDS_AMT       1024

#define DEFAULT_MAIN_THREAD_TO_SEC          0
#define DEFAULT_SERVER_WRITE_THREAD_TO_SEC  0
#define DEFAULT_AGENT_THREAD_TO_SEC         1





#endif //PRESTO_PC_DEFAULTS_H
