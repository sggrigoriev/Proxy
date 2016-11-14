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

/**
 * Internet of Things API connecting physical devices to cloud services
 *
 * @author David Moss
 * @author Andrey Malashenko
 */

#ifndef IOTAPI_H
#define IOTAPI_H

#include <stdbool.h>
#include "ioterror.h"
#include "eui64.h"

enum {
  IOT_COMMAND_NAME_STRING_SIZE = 16,
  IOT_COMMAND_VALUE_STRING_SIZE = 256,
  IOT_COMMAND_TYPE_STRING_SIZE = 10,
};

/**
 * Param types enumerator
 */
typedef enum param_type_e {
  IOT_PARAM_PROFILE,  // deprecated
  IOT_PARAM_MEASURE,
  IOT_PARAM_ALERT,
} param_type_e;

/**
 * Result codes enumerator
 */
typedef enum result_code_e {
  IOT_RESULT_RECEIVED = 0,
  IOT_RESULT_EXECUTED = 1,
  IOT_RESULT_HUBERROR = 2,
  IOT_RESULT_HUBNOTSUPPORTED = 3,
  IOT_RESULT_DEVICENOTIDENTIFIED = 4,
  IOT_RESULT_DEVICENOTSUPPORTED = 5,
  IOT_RESULT_DEVICECONNECTIONERROR = 6,
  IOT_RESULT_DEVICEEXECUTIONERROR = 7,
  IOT_RESULT_WRONGFORMAT = 8,
} result_code_e;


/**
 * Structure to store an individual command
 */
typedef struct command_t {

  /** True when a user is actively watching in real time, send data quickly */
  bool userIsWatching;

  /**
   * True when this command is not valid except for the userIsWatching flag
   * and there are no more commands in this message from the server,
   * so execute all the commands you might have been buffering now.
   */
  bool noMoreCommands;

  /** Command ID to use in a response to the server */
  int commandId;

  /** Destination device ID */
  char deviceId[EUI64_STRING_SIZE];

  /** Type attribute, i.e. "set", "delete", "discover", etc. */
  char commandType[IOT_COMMAND_TYPE_STRING_SIZE];

  /** Index # in ascii */
  char asciiIndex;

  /** Command name */
  char commandName[IOT_COMMAND_NAME_STRING_SIZE];

  /** Command arguments */
  const char *argument;

  /** Arguments size */
  int argSize;


} command_t;

/**
 * Command listener function definitions take on the form:
 *
 *   void doCommand(command_t *cmd)
 *
 * Command listeners are responsible for matching their device ID and command
 * name, etc.
 */
#ifndef __type_commandlistener
#define __type_commandlistener
typedef void (*commandlistener_f)(command_t *);
#endif

/**
 * This function prototype MUST be implemented by the application layer in order
 * to route the outbound message to the appropriate handler
 * @param msg Final message to send
 * @param len Length of the message to send
 */
error_t application_send(const char *msg, int len);

/***************** API ****************/
error_t iotxml_addDevice(const char *deviceId, int deviceType);

error_t iotxml_alertDeviceIsGone(const char *deviceId);

int iotxml_newMsg(char *destMsg, int maxSize);

int iotxml_addString(char *dest, int maxSize, const char *deviceId, int deviceType, param_type_e paramType, const char *paramName, const char *multiplier, char asciiParamIndex, const char *paramValue);

int iotxml_addInt(char *dest, int maxSize, const char *deviceId, int deviceType, param_type_e paramType, const char *paramName, const char *multiplier, char asciiParamIndex, int paramValue);

error_t iotxml_send(char *destMsg, int maxSize);

void iotxml_abortMsg();

error_t iotxml_sendResult(int commandId, result_code_e result);

error_t iotxml_parse(const char *xml, int len);

error_t iotxml_addCommandListener(commandlistener_f l, char *type);

error_t iotxml_removeCommandListener(commandlistener_f l);

error_t iotxml_pushMeasurementNow(const char *deviceId);

#endif
