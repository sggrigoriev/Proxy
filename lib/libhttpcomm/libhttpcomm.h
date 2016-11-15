/*
 *  Copyright 2013 People Power Company
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

#ifndef LIBHTTPCOMM_H
#define LIBHTTPCOMM_H

#include <curl/curl.h>
#include <limits.h>
#include <rpc/types.h>
#include <stdbool.h>

/** Maximum time for an HTTP connection, including name resolving */
#define HTTPCOMM_DEFAULT_CONNECT_TIMEOUT_SEC 30

/** Maximum time for an HTTP transfer */
#define HTTPCOMM_DEFAULT_TRANSFER_TIMEOUT_SEC 60

/** Maximum amount of time before resolving the name again */
#define HTTPCOMM_DEFAULT_DNS_CACHING_TIMEOUT_SEC 120

/** Size of a buffer needed to hold a string describing a port */
#define HTTPCOMM_PORT_STRING_SIZE 6

/** Size of a string needed to hold "true" or "false" for SSL */
#define HTTPCOMM_SSL_STRING_SIZE 6

/** Size of a string needed to hold a authentication token */
#define HTTPCOMM_AUTHENTICATION_STRING_SIZE 128


typedef struct http_timeout_t {
  long connectTimeout;
  long transferTimeout;
} http_timeout_t;


typedef struct http_param_t {
  http_timeout_t timeouts;
  bool verbose;
  char *password;
  char *key;
} http_param_t;


/***************** Public Prototypes *****************/
void libhttpcomm_curlShareInit(CURLSH *curlHandle);

void libhttpcomm_curlShareClose(CURLSH *curHandle);

int libhttpcomm_getMsg(CURLSH * shareCurlHandle, const char *url,
    const char *sslCertPath, const char *authToken, char *rxBuffer,
    int maxRxBufferSize, http_param_t params, int(*ProgressCallback)(
        void *clientp, double dltotal, double dlnow, double ultotal,
        double ulnow));

int libhttpcomm_sendMsg(CURLSH * shareCurlHandle, CURLoption httpMethod,
    const char *url, const char *sslCertPath, const char *authToken,
    char *msgToSendPtr, int msgToSendSize, char *rxBuffer, int maxRxBufferSize,
    http_param_t params, int(*ProgressCallback)(void *clientp, double dltotal,
        double dlnow, double ultotal, double ulnow));

int libhttpcomm_postMsg(CURLSH * shareCurlHandle, CURLoption httpMethod,
    const char *url, const char *sslCertPath, const char *authToken,
    char *msgToSendPtr, int msgToSendSize, char *rxBuffer, int maxRxBufferSize,
    http_param_t params, int(*ProgressCallback)(void *clientp, double dltotal,
        double dlnow, double ultotal, double ulnow));

int libhttpcomm_getFile(CURLSH * shareCurlHandle, const char *url,
    const char *sslCertPath, const char *authToken, FILE *rxfile,
    int maxRxFileSize, http_timeout_t timeouts, int(*ProgressCallback)(
        void *clientp, double dltotal, double dlnow, double ultotal,
        double ulnow));

int libhttpcomm_sendFile(const char *url, const char *sslCertPath,
    const char *authToken, char *fileName, char *rxBuffer, int maxRxBufferSize,
    http_timeout_t timeouts);

#endif
