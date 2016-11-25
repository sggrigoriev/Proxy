//
// Created by gsg on 22/11/16.
//

#include "pc_defaults.h"
#include "pu_logger.h"
#include "pc_settings.h"
#include "pf_proxy_activation.h"

////////////////////////////////////////////////////////
//pf_proxy_activation   - analyze the presence of deviceID and activation token in loaded settings
//
// Possible actions:
//1. Do nothing if activated (both items are found)
//2. Get activation token from the cloud
//3. Generate deviceID and get the activation token from the cloud
//Return 0 if error and 1 if OK
//
int pf_proxy_activation() {  //return 0 if proxy was not activated
    switch(pc_calc_activation_type()) {
        case PC_FULL_ACTIVATION: {                    // Create device ID and store it ni file and in memory
            char eui_string[PROXY_DEVICE_ID_SIZE];

            if(!eui64_toString(eui_string, sizeof(eui_string))) {
                pu_log(LL_ERROR, "pf_proxy_activation: Unable to get the Gateway DeviceID. Activaiton failed");
                return 0;
            }
            if(!pc_saveDeviceAddress(eui_string)) {
                pu_log(LL_WARNING, "pf_proxy_activation: Unable to store Device ID into configuration file");
            }
        }
        case PC_ACTIVATION_KEY_NEEDED:              // Ask from cloud about the activation key and store it in file and in memory
        case PC_NO_NEED_ACTIVATION:                 // Nothing to do, Proxy got everything it needs
            break;
        default:    //total mess
            pu_log(LL_ERROR, "Proxy activation failed. ");
            return 0;
    }
    return 1;
}