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
 */
/*
    Created by gsg on 06/12/16.
    Wrapper thread: wait for messages for Agent and passes it to the Agent trough TCP
    Mainly queue -> TCP forwarder
*/

#ifndef PRESTO_PT_AGENT_WRITE_H
#define PRESTO_PT_AGENT_WRITE_H
/**
 * Start agent write thread
 *
 * @param wrie_socket   - socket for writing
 * @return  - 1
*/
int start_agent_write(int write_socket);

/**
 * Stop agent write thread
*/
void stop_agent_write();


#endif /* PRESTO_PT_AGENT_WRITE_H */
