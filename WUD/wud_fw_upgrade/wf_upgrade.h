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
    Created by gsg on 01/12/16.

    Contains firmware upgrade helpers: file check & status management.
*/

#ifndef PRESTO_WF_UPGRADE_H
#define PRESTO_WF_UPGRADE_H

#define WF_STATE_BUZY 1

//return 1 if check_sum compared with calculated on file (SHA256)
/***********************************************************************************
 * Compute file check sum (SHA256) and compare in twith the given check sum (char representaion)
 * @param check_sum     - given check sum: hexadecimal as a string with two bytes ber each digit
 * @param path          - path to the checked file
 * @param file_name     - file name
 * @return - 1 if check_sum compared with calculated on file, 0 if not
 */
int wf_check_file(const char* check_sum, const char* path, const char* file_name);
/***********************************************************************************
 * Set the download status
 * @param empty     - 0 if no download, 1 if downlowad in process
 */
void wf_set_download_state(int empty);
/***********************************************************************************
 * Set the upgrade status
 * @param empty     - 0 if no upgrade; 1 if upgrade in process
 */
void wf_set_upgrade_state(int empty);
/***********************************************************************************
 * Returns the download status
 * @return - 1 - on, 0 - off
 */
int wf_was_download_empty();
/***********************************************************************************
 * REturn the upgrade status
 * @return - 1 - on, 0 - off
 */
int wf_was_upgrade_empty();

#endif /* PRESTO_WF_UPGRADE_H */
