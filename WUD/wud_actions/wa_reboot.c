//
// Created by gsg on 09/12/16.
//

#include <stdlib.h>
#ifndef WUD_ON_HOST
    #include <sys/reboot.h>
#endif
#include "pu_logger.h"
#include "wa_reboot.h"

void wa_reboot() {   //run WUD (or system?) totel reboot
#ifdef WUD_ON_HOST
    pu_log(LL_INFO, "wa_reboot: EXIT on host case");
    exit(1);
#else
    pu_log(LL_INFO, "wa_reboot: true rebood for true gateway");
    sync();
	reboot(RB_POWER_OFF);
#endif
}