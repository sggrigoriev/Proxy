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
 * Command line Interface for the proxy server
 * @author David Moss
 */


#include <string.h>
#include <stdio.h>
#include <getopt.h>

#include "presto_release_version.h"

#include "pc_defaults.h"
#include "pc_settings.h"
#include "pc_cli.h"

/***************** Private Prototypes ****************/
static void _proxycli_printUsage();
static void _proxycli_printVersion();

static void print_device_id() {
    if(!pc_existsProxyDeviceID()) {
        char eui_string[PROXY_DEVICE_ID_SIZE];
        if (!eui64_toString(eui_string, sizeof(eui_string))) {
            printf("Unable to generate the Gateway DeviceID");
        }
        printf("%s%s", pc_getProxyDeviceIDPrefix(), eui_string);
    }
    else {
        char deviceID[LIB_HTTP_DEVICE_ID_SIZE] = {0};
        pc_getProxyDeviceID(deviceID, sizeof(deviceID));
        printf("%s", deviceID);
    }
}

/***************** Public Functions ****************

 Parse the command line arguments, to be retrieved by getter functions when
 needed and update loadad from  config file parameters + updates the donfig file itself

Return 0 if error. Parse and update config data + loaded params
*/
int pc_cli_process_params(int argc, char *argv[]) { /*Return 0 if error. Parse and update config data + loaded params*/

  int c;

  while ((c = getopt(argc, argv, "v:d:?")) != -1) {
    switch (c) {
    case 'v':
      _proxycli_printVersion();
      return 0;
    case 'd':
         print_device_id();
        return 0;
        break;
    case '?':
      _proxycli_printUsage();
      return 0;
     default:
      printf("[cli] Unknown argument character code 0%o\n", c);
      _proxycli_printUsage();
      return 0;
    }
  }
  return 1;
}

/***************** Private Functions ****************/
/**
 * Instruct the user how to use the application
 */
static void _proxycli_printUsage() {
  char *usage = ""
    "Usage: ./Proxy (options)\n"
    "\t[-v] : Print version information\n"
    "\t[-d] : Print deviceID\n"
    "\t[-?] : Print this menu\n"
    "\n";

  printf("%s", usage);
}
/**
 * Print the version number
 */
static void _proxycli_printVersion() {
  printf("Built on %s at %s\n", __DATE__, __TIME__);
  printf("Git repository version %s\n", PRESTO_FIRMWARE_VERSION
  );
}
