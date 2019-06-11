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
 *

 Created by gsg on 13/12/16.

 Set of helpers to communicate with cloud
 Has to be refucktored.
*/

#ifndef PRESTO_PF_TRAFFIC_PROC_H
#define PRESTO_PF_TRAFFIC_PROC_H

#include <stdlib.h>
#include <cJSON.h>

/*
    https://iotdevices.docs.apiary.io/#introduction/hardware-to-cloud-result-codes
*/
typedef enum {PF_RC_ACK = 0, PF_RC_OK = 1, PF_RC_UNSUPPORTED = 5, PF_RC_EXEC_ERR = 7, PF_RC_FMT_ERR = 8} t_pf_rc;

/**
 * Extract the value of "commandId" field from parsed cloud message
 *
 * @param cmd   - parsed JSON message
 * @return  - 0 if error, commandId value if OK
 */
unsigned long pf_get_command_id(cJSON* cmd);
/**
 * Creates string with the answer for command execution as
 * {"responses": [{"commandId": <command_id>, "result": <RC>}]}
 *
 * @param cmd       - parsed message with command
 * @param buf       - buffer to store the string created
 * @param buf_size  - buffer siza
 * @param rc        - rc to be set into answer
 * @return  - empty string if error or response string if Ok
 */
const char* pf_answer_to_command(cJSON* cmd, char* buf, size_t buf_size, t_pf_rc rc);
/**
 * Creates string with the answer for command execution as by commandId value
 * {"responses": [{"commandId": <command_id>, "result": <RC>}]}
 *
 * @param cmd_id    - command_id
 * @param buf       - buffer tp store answer string
 * @param buf_size  - buffer size
 * @param rc        - RC of operation
 * @return  - Return empty string or answer string
 */
const char* pf_make_answer_to_command(unsigned long cmd_id, char* buf, size_t buf_size, t_pf_rc rc);

/**
 * Make list as &cmdId=<commandId>&...&cmdId=<commandId> - quick answer "command received sent with next long GET
 *
 * @param root  - parsed JSON with commands list
 * @return  - NULL if error or construted string.
 *          NB! returned buffer must be freed!
 */
char* pf_make_cmds_list(cJSON* root);


/**
 * Add head required by cloud protocol The sequential number will be assigned inside
 * "proxyId": "<proxy_id", "sequenceNumber": "<seq_number>"
 *
 * @param msg       - the message in JSON format to be sent
 * @param msg_size  - buffer's containing message size
 * @param device_id - string with the gateway device id
 * @return  - updated message length
*/
size_t pf_add_proxy_head(char* msg, size_t msg_size, const char* device_id);

#endif /*PRESTO_PF_TRAFFIC_PROC_H */
