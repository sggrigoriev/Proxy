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
    Created by gsg on 09/12/16.
    Gateway reboot official place. For the "HOST" configuration makes exit instead of reboot.
*/

#ifndef PRESTO_WA_REBOOT_H
#define PRESTO_WA_REBOOT_H

/**
 * Run system reboot. For host - just exit. If "REBOOT_BY_REQUEST" set to 0 - exit in any case
 *
 * @param exit_rc   - used as EXIT RC code:
 *                  - 0 just exit
 *                  - 1 if some child hangs on
 *                  - 2 if FW upgrade finished
 *                  - 3 if WUD has unrecovered error.
 *                  See wc_defaiults.h WUD_DEFAULT_EXIT_* defines
*/

void wa_reboot(int exit_rc);

#endif /* PRESTO_WA_REBOOT_H */
