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

#ifndef PROXY_H
#define PROXY_H

#include "ioterror.h"
#include "proxylisteners.h"

enum {
  PROXY_MAX_HTTP_RETRIES = 3,
  PROXY_MAX_MSG_LEN = 16384,
  PROXY_NUM_SERVER_CONNECTIONS_BEFORE_SYSLOG_NOTIFICATION = 20,
  PROXY_MAX_PUSHES_ON_RECEIVED_COMMAND = 2,
  PROXY_HEADER_PASSWORD_LEN = 64,
  PROXY_HEADER_KEY_LEN = 256,
};

/**************** Public Prototypes ****************/
error_t proxy_start(const char *url);

void proxy_stop();

error_t proxy_addListener(proxylistener l);

error_t proxy_removeListener(proxylistener l);

error_t proxy_send(const char *data, int len);

#endif

