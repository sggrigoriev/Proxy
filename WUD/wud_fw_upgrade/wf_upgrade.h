//
// Created by gsg on 01/12/16.
//

#ifndef PRESTO_WF_UPGRADE_H
#define PRESTO_WF_UPGRADE_H

//Make firmware upgrade task
//Return 1 if OK, 0 if error

typedef enum {WF_LOAD_FAILED, WF_CHECK_FAILED, WF_OK} wf_upgrade_rc_t;

void wf_upgrade(const char* upgrade_command);

//FW Upgrade reqwest processor part
//Return the final FW Upgrade state
wf_upgrade_rc_t wf_rp_upgrade(const char* alert);

#endif //PRESTO_WF_UPGRADE_H
