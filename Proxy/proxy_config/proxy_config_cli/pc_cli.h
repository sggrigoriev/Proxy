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

#ifndef PROXYCLI_H
#define PROXYCLI_H

/* pc_cli_process_params RC */

typedef enum {PCLI_UNDEF, PCLI_VERSION, PCLI_DEVICE_ID} pc_cli_action_t;
typedef struct {
    pc_cli_action_t action;
    char* parameter;
} pc_cli_params_t;


/***************** Public Prototypes ****************/
/* Process the parameters passed on Proxy start
 *  Return actions requested array terminated by PCLI_SIZE
 */
pc_cli_params_t pc_cli_process_params(int argc, char *argv[]);

/**
 * Instruct the user how to use the application
 */
void pc_cli_printUsage();

/**
 * Print the version number
 */
void pc_cli_printVersion();

void pc_cli_printDeviceID();


#endif

