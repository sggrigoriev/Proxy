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
    Created by gsg on 07/12/16.

    Receives messages from WUD threads and writes 'em to the cloud
*/

#ifndef PRESTO_WT_SERVER_WRITE_H
#define PRESTO_WT_SERVER_WRITE_H

/***********************************
 * Start thread function
 * @return  - 1 if OK, 0 if not
 */
int wt_start_server_write();

/***********************************
 * Stop thread function
 */
void wt_stop_server_write();

/***********************************
 * Set stop flag ON for assync stop
 */
void wt_set_stop_server_write();

#endif /* PRESTO_WT_SERVER_WRITE_H */
