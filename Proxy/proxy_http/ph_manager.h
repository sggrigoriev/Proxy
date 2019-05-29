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
    Created by gsg on 13/11/16.
    Contains data and functions to communicate with the Cloud - Proxy wrapper to libhttpcomm
*/

#ifndef PH_MANAGER_H
#define PH_MANAGER_H

#include "lib_http.h"

/***************************************************
 * Initiates PH manager, no cloud connections are made
 */
void ph_mgr_init();
/***************************************************
 * Close all existing connections, deinit PH manager
*/
void ph_mgr_destroy();

/*
    Case when the main url came from the cloud
1. Get contact url from main
2. Get auth token
3. Save existing main&contact URLs and auth token for further notification
4. Reopen connections
5. save new main & new contact & auth token
if error - close everything and make the step "initiation connections"; return 0
if reboot requested return -1
if OK return 1
*/
int ph_update_main_url(const char* new_main);

/*
    Case of connection error
0. Check if new_main in the list of allowed domains
1. Get contact url
2. Reopen connections
3. if error start from 1
4. save new contact url
 Returns 1 if it was true reconnect and returns 0 if it was secondary call
 Returns -1 if reboot required
*/
int ph_reconnect();

/*
    Case of periodic update the contact url
1. Get contact url from main url
2. Reopen connections
If error - open connections with previous contact url
If error again - start from step 1
3. Save new contact url
 Return 1 if OK
 Return -1 if reboot required
*/
int ph_update_contact_url();

/* Communication functions */

/* Read into in_buf the message from the cloud (GET). Answer ACK(s) id command(s) came
 *      answers - string with commands Id list made from commands received on previous call
 *      in_buf  - buffer for data received
 *      size    - buffer size
 *  Return  0 if timeout, 1 if OK, -1 if error - reconnect required
*/
int ph_read(const char* answers, char* in_buf, size_t size);

/* POST the data to cloud; receive (possibly) the answer
 *      buf         - message to be sent (0-terminated string)
 *      resp        - buffer for cloud respond
 *      rest_size   - buffer size
 *  Returns 0 if error, 1 if OK
*/
int ph_write(char* buf, char* resp, size_t resp_size);

/* Same as previous. But uses different connection descriptor for immediate response */
/* this is the reason of duplication - they will be used in parallel in separate threads */
int ph_respond(char* buf, char* resp, size_t resp_size);

#endif /* PH_MANAGER_H */
