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
    Read data from the cloud (long GET) and send to main proxy and to the agent write threads.
    Mainly the HTTP(s)-> queue wrapper
*/

#ifndef PRESTO_PT_SERVER_READ_H
#define PRESTO_PT_SERVER_READ_H

/**
 * Start the thread
 */
int start_server_read();

/**
 * Stop the thread
*/
void stop_server_read();

/**
 * Switch on the stop flag - async stop
*/
void set_stop_server_read();

#endif /* PRESTO_PT_SERVER_READ_H */
