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

Add the head to the message sent to the cloud:
  {"proxtId":"<deviceId>, "sqeuenceNunber": <sqe_number>, <msg>}
*/

#ifndef PRESTO_PF_TRAFFIC_PROC_H
#define PRESTO_PF_TRAFFIC_PROC_H

#include <stdlib.h>
/*Add head required by cloud protocol
  msg         - the message in JSON format to be sent
  msg_size    - buffer's containing message size
  device_id   - string with the gateway device id
  seq_number  - message squential number (add hock one) - cloud does not care about this
Return updated message length
*/
size_t pf_add_proxy_head(char* msg, size_t msg_size, const char* device_id, unsigned int seq_number);

#endif /*PRESTO_PF_TRAFFIC_PROC_H */
