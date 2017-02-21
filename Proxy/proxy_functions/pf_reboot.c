//
// Created by gsg on 28/01/17.
//

#include <stdlib.h>
#ifndef PROXY_ON_HOST
    #include <sys/reboot.h>
#endif

#include "pu_logger.h"
#include "pf_reboot.h"

//The functione call total revice reboot in case of hard non-compenstive internal
void pf_reboot() {
#ifdef PROXY_ON_HOST
    pu_log(LL_INFO, "pf_reboot: EXIT on host case");
    exit(1);
#else
    pu_log(LL_INFO, "pf_reboot: true rebood for true gateway");
    sync();
    reboot(RB_POWER_OFF);
#endif
}
