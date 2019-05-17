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
*/

#include <stdlib.h>
#include <stdio.h>
#include <wc_settings.h>
#include "pu_logger.h"
#include "wa_manage_children.h"

#ifndef WUD_ON_HOST
    #include <sys/reboot.h>
#endif

#include "wa_reboot.h"


void wa_reboot() {
    if(wc_getRebootByRequest()) {
#ifdef WUD_ON_HOST
        pu_log(LL_INFO, "wa_reboot: EXIT on host case");
        wa_stop_children();
        fprintf(stdout, "wa_reboot: EXIT on host case\n");
        exit(1);
#else
        pu_log(LL_INFO, "wa_reboot: reboot!");
        fprintf(stdout, "wa_reboot: reboot!");
        sync();
#ifdef PLATFORM_HISILICON_C1
        reboot(RB_AUTOBOOT);
#else
        reboot(RB_POWER_OFF);
#endif
#endif
    }
    else {
        pu_log(LL_INFO, "%s: Reboot disabled by WUD configuration. Exiting. Refer to the \"REBOOT_BY_REQUEST\" setting.", __FUNCTION__);
        wa_stop_children();
        fprintf(stdout, "%s: Reboot disabled by WUD configuration. Exiting. Refer to the \"REBOOT_BY_REQUEST\" setting.\n", __FUNCTION__);
        exit(1);
    }
}