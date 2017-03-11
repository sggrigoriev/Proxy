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
    Forwards info from to the cloud. Sends answer to the agent write thread
*/

#ifndef PRESTO_PT_SERVER_WRITE_H
#define PRESTO_PT_SERVER_WRITE_H

int start_server_write();

void stop_server_write();

void set_stop_server_write();

#endif /* PRESTO_PT_SERVER_WRITE_H */
