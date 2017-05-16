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
    Created by gsg on 30/01/17.

    HTTP connections manager. Specially for WUD.
*/

#ifndef PRESTO_WH_MANAGER_H
#define PRESTO_WH_MANAGER_H

#include <stdio.h>

#include "lib_http.h"

void wh_mgr_init();     /* Initiates httplib but not creates the connections */
void wh_mgr_destroy();  /* Deletes connections (if any and deinit httplib */

/********************************************************************************
 * (re)create post connection; update conn parameters
 */
void wh_reconnect();

/********************************************************************************
 * POST a message to the cloud; receive anser (if any) or error
 * @param buf           - buffer with the message to be sent
 * @param resp          - buffer to receive the response
 * @param resp_size     - response buffer size
 * @return  - 0 if error, 1 if OK
 */
int wh_write(char* buf, char* resp, size_t resp_size);

/*Returns 0 if error, 1 if OK
********************************************************************************
 * Download the file from the cloud
 * @param file_with_path    - the place to store the file downloaded
 * @param url               - file's URL
 * @param attempts_amount   - download attempts before total failure
 * @return  - 1 if OK, 0 if not
 */
int wh_read_file(const char* file_with_path,  const char* url, unsigned int attempts_amount);

#endif /* PRESTO_WH_MANAGER_H */
