//
// Created by gsg on 21/11/16.
//

#include <stdio.h>
#include <stdlib.h>

#include "eui64.h"
#include "pc_settings.h"
#include "pc_defaults.h"
#include "pc_cli.h"

int main(int argc, char *argv[]) {
    char buf[4097];

        if (!pc_cli_process_params(argc, argv)) return 1;

//    if(!pc_load_config("/media/sf_GWDrive/Presto_new/build/proxyJSON.test")) return 1;
        pu_start_logger(DEFAULT_LOG_NAME, 3, LL_DEBUG);
        pu_log(LL_WARNING, "1111111111111111111111111111111111111111111111111");

        printf("getLogFileName(): %s\n", pc_getLogFileName());
        printf("getLogRecordsAmt(): %d\n", pc_getLogRecordsAmt());
        printf("getLogVevel(): %d\n", pc_getLogVevel());
        printf("getSertificatePath(): %s\n", pc_getSertificatePath());
        pc_getActivationToken(buf, sizeof(buf));
        printf("getActivationToken(): %s\n", buf);
        pc_getDeviceAddress(buf, sizeof(buf));
        printf("getDeviceAddress(): %s\n", buf);
        printf("pc_getProxyDeviceType(): %d\n", pc_getProxyDeviceType());
        printf("getLongGetTO(): %d\n", pc_getLongGetTO());
        printf("getAgentPort(): %d\n", pc_getAgentPort());
        printf("getQueuesRecAmt(): %d\n", pc_getQueuesRecAmt());
        pc_getCloudURL(buf, sizeof(buf));
        printf("pc_getCloudURL(): %s\n", buf);

        printf("\n\n------------------------------------------------------\n");
        if (!pc_saveActivationToken("LsW2zj2GbRvvRGWyorjRbG5ZZSD6LvP1VRCKQIME678=")) return 1;
        pc_getActivationToken(buf, sizeof(buf));
        printf("getActivationToken(): %s\n", buf);
        printf("\n\n------------------------------------------------------\n");
        char eui_string[PROXY_DEVICE_ID_SIZE];

        if (!eui64_toString(eui_string, sizeof(eui_string))) {
            pu_log(LL_ERROR, "pf_proxy_activation: Unable to get the Gateway DeviceID. Activaiton failed");
            return 0;
        }
        if (!pc_saveDeviceAddress(eui_string)) {
            pu_log(LL_WARNING, "pf_proxy_activation: Unable to store Device ID into configuration file");
        }
    pu_log(LL_INFO, "22222222222222222222222222222");
    pu_log(LL_INFO, "333333333333333333333333333333333333");

    return 0;
}
