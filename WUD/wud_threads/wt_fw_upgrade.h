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

    Firmware upgrade thread.
        - Download file
        - check sha256
        - copy to install directory, rename, delete source
        - finish

*/

#ifndef PRESTO_WT_FW_UPGRADE_H
#define PRESTO_WT_FW_UPGRADE_H

#include "pr_commands.h"

/**
 * Start thread
 *
 * @param fwu_start - file URL + check summ
 * @return  - 1 if OK, 0 if not
 */
int wt_start_fw_upgrade(pr_cmd_fwu_start_t fwu_start);

/**
 * Stop thread
 */
void wt_stop_fw_upgrade();

/**
 * Set stop flag: async stop
 */
void wt_set_stop_fw_upgrade();


#endif /* PRESTO_WT_FW_UPGRADE_H */
