//
// Created by gsg on 07/12/16.
//

#ifndef PRESTO_WT_FW_UPGRADE_H
#define PRESTO_WT_FW_UPGRADE_H

#include "pr_commands.h"

int wt_start_fw_upgrade(pr_cmd_fwu_start_t fwu_start);

void wt_stop_fw_upgrade();

void wt_set_stop_fw_upgrade();


#endif //PRESTO_WT_FW_UPGRADE_H
