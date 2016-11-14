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
 * This module is responsible for parsing commands from the server
 * @author David Moss
 */

#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdlib.h>
#include <stdbool.h>
#include <rpc/types.h>
#include <stdio.h>

#include <libxml/parser.h>

#include "iotapi.h"
#include "ioterror.h"
#include "iotdebug.h"
#include "iotcommandlisteners.h"
#include "iotparser.h"
#include "eui64.h"
#include "upgrade.h"


/** True if a param tag was found in the command */
static bool paramTagFound;

/***************** Private Prototypes ****************/
static void _iotparser_xml_startElementHandler(void *ctx, const xmlChar *name, const xmlChar **atts);

static void _iotparser_xml_endElementHandler(void *ctx, const xmlChar *name);

static void _iotparser_xml_charactersHandler(void *ctx, const xmlChar *ch, int len);

upgrade_t upgrade;

/***************** Public Functions ****************/
error_t iotxml_parse(const char *xml, int len) {
  command_t command;
  memset(&command, 0x0, sizeof(command));
  memset(&upgrade, 0x0, sizeof(upgrade));

  xmlSAXHandler saxHandler = {
      NULL, // internalSubsetHandler,
      NULL, // isStandaloneHandler,
      NULL, // hasInternalSubsetHandler,
      NULL, // hasExternalSubsetHandler,
      NULL, // resolveEntityHandler,
      NULL, // getEntityHandler,
      NULL, // entityDeclHandler,
      NULL, // notationDeclHandler,
      NULL, // attributeDeclHandler,
      NULL, // elementDeclHandler,
      NULL, // unparsedEntityDeclHandler,
      NULL, // setDocumentLocatorHandler,
      NULL, // startDocument
      NULL, // endDocument
      _iotparser_xml_startElementHandler, // startElement
      _iotparser_xml_endElementHandler, // endElement
      NULL, // reference,
      _iotparser_xml_charactersHandler, //characters
      NULL, // ignorableWhitespace
      NULL, // processingInstructionHandler,
      NULL, // comment
      NULL, // warning
      NULL, // error
      NULL, // fatal
  };

  command.userIsWatching = (strstr(xml, "CONT") != NULL);

  SYSLOG_DEBUG("Parsing XML: %s", xml);
  if(0 != xmlSAXUserParseMemory(&saxHandler, &command, xml, len)) {
    SYSLOG_ERR("Couldn't parse XML");
    return FAIL;
  }

  return SUCCESS;
}

/***************** Private Functions ****************/
/**
 * XML start element handler
 */
static void _iotparser_xml_startElementHandler(void *ctx, const xmlChar *name, const xmlChar **atts) {
  int i;
  char *attr;
  char *value;
  command_t *command = (command_t *) ctx;


  if(strcmp((char *) name, IOTPARSER_TAG_COMMAND) == 0) {
    // New command, clear out all the residual command and argument information
    paramTagFound = false;
    bzero(command->deviceId, EUI64_STRING_SIZE);
    bzero(command->commandName, IOT_COMMAND_NAME_STRING_SIZE);
    command->commandId = -1;
    command->asciiIndex = 0;
    command->argument = NULL;
    command->argSize = 0;

  } else if(strcmp((char *) name, IOTPARSER_TAG_PARAM) == 0) {
    // New parameter, clear out the residual argument information but leave
    // everything else intact
    paramTagFound = true;
    command->asciiIndex = 0;
    command->argument = NULL;
    command->argSize = 0;
  }

  if (atts != NULL) {
    for (i = 0; (atts[i] != NULL); i++) {
      attr = (char *) atts[i++];
      value = (char *) atts[i];
      SYSLOG_DEBUG("attr: (%s) value: (%s)", attr, value);

      if(strcmp(attr, IOTPARSER_ATTR_COMMANDID) == 0) {
        command->commandId = atoi(value);

      } else if(strcmp(attr, IOTPARSER_ATTR_DEVICEID) == 0) {
        strncpy(command->deviceId, value, EUI64_STRING_SIZE);

      } else if(strcmp(attr, IOTPARSER_ATTR_INDEX) == 0) {
        command->asciiIndex = atoi(value) + '0';

      } else if(strcmp(attr, IOTPARSER_ATTR_COMMANDTYPE) == 0) {
        strncpy(command->commandType, value, IOT_COMMAND_TYPE_STRING_SIZE);

        // Command types always appear before command names.
        // If this command happens to not have a name, then the default
        // is to make the command name the same as the command type.
        strncpy(command->commandName, value, IOT_COMMAND_NAME_STRING_SIZE);

      } else if(strcmp(attr, IOTPARSER_ATTR_COMMANDNAME) == 0) {
        // If the parameter has a name, then that is the real command name
        strncpy(command->commandName, value, IOT_COMMAND_NAME_STRING_SIZE);
      }
    }
  }
}

/**
 * XML end element handler
 */
static void _iotparser_xml_endElementHandler(void *ctx, const xmlChar *name) {
  if(strcmp((char *) name, IOTPARSER_TAG_S2H) == 0) {
    command_t *command = (command_t *) ctx;

    // Send out a command to all listeners that there are no more commands
    // This is useful when we might receive several commands that we buffered
    // up because they need to execute simultaneously
    //
    // Most modules should just ignore commands with
    // ID -1 or noMoreCommands=true
    //
    // Devices like a multi-socket powerstrip might be interested in this
    // because it would send a single command to the end device telling it
    // to switch multiple sockets at once.
    command->noMoreCommands = true;
    bzero(command->deviceId, EUI64_STRING_SIZE);
    bzero(command->commandName, IOT_COMMAND_NAME_STRING_SIZE);
    command->asciiIndex = 0;
    command->commandId = -1;
    command->argument = NULL;
    command->argSize = 0;

    iotcommandlisteners_broadcast(command);

  } else if(strcmp((char *) name, IOTPARSER_TAG_PARAM) == 0) {
    // This is the end of a param tag
    paramTagFound = true;
    iotcommandlisteners_broadcast((command_t *) ctx);

  } else if(strcmp((char *) name, IOTPARSER_TAG_COMMAND) == 0 && !paramTagFound) {
    // This is the end of a command tag where there were no param tags within it
    iotcommandlisteners_broadcast((command_t *) ctx);
  }
}


/**
 * XML character handler
 */
static void _iotparser_xml_charactersHandler(void *ctx, const xmlChar *ch, int len) {
  command_t *command = (command_t *) ctx;
  command->argument = (char *) ch;
  command->argSize = len;
  if ( strncmp("ppm.", command->commandName, 4) == 0 )
  {
      char *less = strstr(command->argument, "<");
      char *value = NULL;

      if ( less != NULL ) 
      {
	  value = strndup(command->argument, less - command->argument);
      }

      if ( value != NULL )
      {
	  if ( strcmp(command->commandName, "ppm.action") == 0 )
	  {
	      upgrade.action = strdup(value);
	  }
	  else if  ( strcmp(command->commandName, "ppm.url") == 0 )
	  {
	      upgrade.url = strdup(value);
	  }
	  else if  ( strcmp(command->commandName, "ppm.size") == 0 )
	  {
	      upgrade.size = atoll(value);
	  }
	  else if  ( strcmp(command->commandName, "ppm.md5") == 0 )
	  {
	      upgrade.md5 = strdup(value);
	  }
	  else if  ( strcmp(command->commandName, "ppm.name") == 0 )
	  {
	      upgrade.name = strdup(value);
	  }

	  if ( upgrade.action != NULL && upgrade.url != NULL && upgrade.size != 0 && upgrade.md5 != NULL )
	  {
	      system_upgrade(&upgrade);
	  }

	  free(value);
      }
  }
}


