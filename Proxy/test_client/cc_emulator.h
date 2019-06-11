/*
 *  Copyright 2018 People Power Company
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
/*
 Created by gsg on 19/05/19.

 Emulates the simplest Agent logic - device paring
 Could grow to the big amd powerful emulator... Sometime
*/

#ifndef PRESTO_CC_EMULATOR_H
#define PRESTO_CC_EMULATOR_H

#include <stdio.h>

/**
 * Check if the message has commands
 *
 * @param in_msg    - JSON string
 * @return  - 0 it is not a command
 *          - 1 it contains command
 */
int is_command(const char* in_msg);

/**
 * Check if it is the Permit Join command
 *
 * @param in_msg    - JSON string
 * @return  - 0 alien command
 *          - 1 Permit Join command
 */
int is_eateable(const char* in_msg);

/**
 * Prepare Agent answer for permit join command
 *
 * @param in_msg    - incoming Permit Join command
 * @param out_msg   - buffer for answer
 * @param max_len   - buffer length
 * @return  - pointer to the out_msg - empty if error
 */
const char* make_answer(const char* in_msg, char* out_msg, size_t max_len);

/**
 * Prepare mesure message: request for permit join from Agent to cloud
 * @param out_buf   - bufer for message
 * @param max_len   - buffer length
 * @return  - pointer to the out_buf
 */
const char* get_mesure(char* out_buf, size_t max_len);

/**
 * Create the answer to the cloud "command received. Obsolete. Now it is Proxy responsibility (again)
 * @param in_buf    - incoming command from cloud
 * @param out_buf   - buffer for answer
 * @param max_len   - bufer lenght
 * @return  - pointer to the buffer
 */
const char* make_0_answer(const char* in_buf, char* out_buf, size_t max_len);

#endif /* PRESTO_CC_EMULATOR_H */
