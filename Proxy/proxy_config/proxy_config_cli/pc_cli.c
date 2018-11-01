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
#include "pc_config.h"
#include "pc_defaults.h"
#include "pc_settings.h"
#include "pc_cli.h"


void pc_cli_printDeviceID() {
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

/***************** Public Prototypes ****************/
/* Process the parameters passed on Proxy start
 *  Return actions requested array terminated by PCLI_SIZE
 */
pc_cli_params_t pc_cli_process_params(int argc, char *argv[]) {
    int c;
    pc_cli_params_t ret;

    ret.parameter = NULL;
    ret.action = PCLI_UNDEF;
    while ((c = getopt(argc, argv, "p:vd")) != -1) {
        switch (c) {
            case 'v':
                ret.action = PCLI_VERSION;
                break;
            case 'd':
                ret.action = PCLI_DEVICE_ID;
                break;
            case 'p':
                ret.parameter = optarg;
                break;
            default:
                break;
        }
    }
  return ret;
}

/**
 * Instruct the user how to use the application
 */
void pc_cli_printUsage() {
  char *usage = ""
    "Usage: ./Proxy (options)\n"
    "\t[-v] : Print version information\n"
    "\t[-d] : Print deviceID\n"
    "\t[-p<path>] : Set path with name for configuration file\n"
    "\t print this menu\n"
    "\n";

  printf("%s", usage);
}
/**
 * Print the version number
 */
void pc_cli_printVersion() {
    char buf[1024]={0};
    printf("%s", get_version_printout(PRESTO_FIRMWARE_VERSION, buf, sizeof(buf)));
}
