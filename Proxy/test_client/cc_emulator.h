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
*/

#ifndef PRESTO_CC_EMULATOR_H
#define PRESTO_CC_EMULATOR_H

#include <stdio.h>

int is_eateable(const char* in_msg);
const char* make_answer(const char* in_msg, char* out_msg, size_t max_len);
const char* get_mesure(char* out_buf, size_t max_len);

#endif /* PRESTO_CC_EMULATOR_H */
