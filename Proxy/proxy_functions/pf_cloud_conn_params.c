//
// Created by gsg on 22/11/16.
//

#include <string.h>

#include "pc_defaults.h"
#include "pu_logger.h"
#include "pc_settings.h"
#include "ph_manager.h"
#include "pf_cloud_conn_params.h"

////////////////////////////////////////////////////////
//pf_get_cloud_conn_params   - get contact point URL & activation token from the cloud
//
// Actions:
//1. Get ProxyDeviceID
//2. Get cloud connection parameters:
//Return 0 if error and 1 if OK
//
int pf_get_cloud_conn_params() {  //return 0 if proxy was not activated
//1. Get ProxyDeviceID
    if(!pc_existsProxyDeviceID()) {
        char eui_string[PROXY_DEVICE_ID_SIZE];
        if(!eui64_toString(eui_string, sizeof(eui_string))) {
            pu_log(LL_ERROR, "pf_get_cloud_conn_params: Unable to get the Gateway DeviceID. Activaiton failed");
            return 0;
        }
        if(!pc_saveProxyDeviceID(eui_string)) {
            pu_log(LL_WARNING, "pf_get_cloud_conn_params: Unable to store Device ID into configuration file");
        }
        else {
            pu_log(LL_INFO, "Proxy Device ID %s succesfully generated", eui_string);
        }
    }
    return 1;
}
