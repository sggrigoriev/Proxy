//
// Created by gsg on 22/11/16.
//
//Local eut test
//
#include <memory.h>

#include "pc_settings.h"
#include "pc_defaults.h"

int main(void) {
    char eui[PROXY_DEVICE_ID_SIZE];

    memset(eui, 0, sizeof(eui));
    if(!pc_load_config(DEFAULT_CFG_FILE_NAME)) {
        printf("Configuration load failed\n");
        exit(-1);
    }
    pu_start_logger(NULL, 5000, LL_DEBUG);

    eui64_toString(eui, sizeof(eui));

    printf("EUI = %s\n", eui);
}
