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
    Created by gsg on 22/11/16.
*/

#include <string.h>

#include "pu_logger.h"

#include "pc_defaults.h"
#include "pc_settings.h"
#include "ph_manager.h"

#include "pf_cloud_conn_params.h"

/*
Return 0 if error and 1 if OK
*/
int pf_get_cloud_conn_params() {  /* return 0 if proxy was not activated */
/* 1. Get ProxyDeviceID */
    if(!pc_existsProxyDeviceID()) {
        char eui_string[PROXY_DEVICE_ID_SIZE];
        char device_id[PROXY_DEVICE_ID_SIZE];
        if(!eui64_toString(eui_string, sizeof(eui_string))) {
            pu_log(LL_ERROR, "pf_get_cloud_conn_params: Unable to get the Gateway DeviceID. Activaiton failed");
            return 0;
        }
        snprintf(device_id, PROXY_DEVICE_ID_SIZE-1, "%s%s", pc_getProxyDeviceIDPrefix(), eui_string);
        if(!pc_saveProxyDeviceID(device_id)) {
            pu_log(LL_WARNING, "pf_get_cloud_conn_params: Unable to store Device ID into configuration file");
        }
        else {
            pu_log(LL_INFO, "Proxy Device ID %s successfully generated", device_id);
        }
    }
    return 1;
}
