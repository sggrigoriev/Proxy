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
#include <getopt.h>

#include "pc_defaults.h"
#include "pc_settings.h"
#include "pc_cli.h"
/***************** Private Prototypes ****************/
static void _proxycli_printUsage();
static void _proxycli_printVersion();

/***************** Public Functions ****************/
/**
 * Parse the command line arguments, to be retrieved by getter functions when
 * needed and update loadad from  config file parameters + updates the donfig file itself
 */
int pc_cli_process_params(int argc, char *argv[]) { //Return 0 if error. Parse and update config data + loaded params

  int c;
  char cfg_fname[PROXY_MAX_PATH];
  char url[PROXY_MAX_PATH];
  int agent_port;
  char activation_key[PROXY_MAX_ACTIVATION_TOKEN_SIZE];

  memset(url, 0, sizeof(url));
  memset(cfg_fname, 0, sizeof(cfg_fname));
  memset(activation_key, 0, sizeof(activation_key));
  agent_port = 0;

  while ((c = getopt(argc, argv, "b:p:c:n:a:v")) != -1) {
    switch (c) {
      case 'b':
        strncpy(url, optarg, sizeof(url)-1);
       break;
    case 'c':
        strncpy(cfg_fname, optarg, sizeof(cfg_fname)-1);
        break;
    case 'n':
      agent_port = atoi(optarg);
      break;
    case 'a':
      strncpy(activation_key, optarg, sizeof(activation_key)-1);
      break;
    case 'v':
      _proxycli_printVersion();
      return 0;
    case '?':
      _proxycli_printUsage();
      return 0;
     default:
      printf("[cli] Unknown argument character code 0%o\n", c);
      _proxycli_printUsage();
      return 0;
    }
  }
  if(strlen(cfg_fname) && pc_saveCfgFileName(cfg_fname)) {
    printf("[cli] New configuration file: %s\n", cfg_fname);
     pc_load_config(cfg_fname);
  }
  else {
    pc_load_config(DEFAULT_CFG_FILE_NAME);
  }
  if(strlen(url) && pc_saveMainCloudURL(url)) {
    printf("[cli] New main cloud URL: %s\n", url);
  }
  if(strlen(activation_key) && pc_saveActivationToken(activation_key)) {
    printf("[cli] New activation key: %s\n", activation_key);
  }
  if(agent_port && pc_saveAgentPort(agent_port)) {
    printf("[cli] New port for Agent connection: %d\n", agent_port);
  }
  return 1;
}

/***************** Private Functions ****************/
/**
 * Instruct the user how to use the application
 */
static void _proxycli_printUsage() {
  char *usage = ""
    "Usage: ./proxyserver (options)\n"
    "\t[-b applicationUrl] : applicationUrl\n"
    "\t[-n port] : Define the port to open the proxy on\n"
    "\t[-c filename] : The name of the configuration file for the proxy\n"
    "\t[-a key] : Activate this proxy using the given activation key and exit\n"
    "\t[-v] : Print version information\n"
    "\t[-?] : Print this menu\n"
    "\n";

  printf("%s", usage);
}
/**
 * Print the version number
 */
static void _proxycli_printVersion() {
  printf("Built on %s at %s\n", __DATE__, __TIME__);
  printf("Git repository version %x\n", GIT_FIRMWARE_VERSION);
}
