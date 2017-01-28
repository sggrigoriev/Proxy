//
// Created by gsg on 28/01/17.
//

#include <stdlib.h>

#include "pu_logger.h"
#include "pf_reboot.h"

//The functione call total revice reboot in case of hard non-compenstive internal
void pf_reboot() {
    pu_log(LL_WARNING, "Presto is going to reboot");
    exit(-1);
}
