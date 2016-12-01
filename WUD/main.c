//
// Created by gsg on 29/11/16.
//

#include <stdio.h>
#include <stdlib.h>
#include <zconf.h>
#include <errno.h>
#include <string.h>
#include <wm_childs_info.h>
#include <wa_manage_children.h>

#include "wc_defaults.h"
#include "wc_settings.h"
#include "wu_utils.h"
#include "wf_dirs_state.h"

int main() {
    printf("WUD launcher start\n");

//WUD must check if /var/run/wud.pid exists
    if(wu_process_exsists(WC_DEFAULT_WUD_NAME)) {
        fprintf(stderr, "WUD is already running. Abort\n");
        exit(1);
    }
//create pid file (var/run/wud.pid
    if(!wu_create_pid_file(WC_DEFAULT_WUD_NAME, getpid())) {
        fprintf(stderr, "Can\'t create PID file for %s: %d %s. Abort\n", WC_DEFAULT_WUD_NAME, errno, strerror(errno));
        exit(1);
    }
//read configuration file
    if(!wc_load_config(WD_DEFAULT_CFG_FILE_NAME)) {
        fprintf(stderr, "Can\'t read configuration file %s: %d %s. Abort.\n", WD_DEFAULT_CFG_FILE_NAME, errno, strerror(errno));
        exit(1);
    }

//initiate logger
    pu_start_logger(/*wc_getLogFileName()*/NULL, wc_getLogRecordsAmt(), wc_getLogVevel());

//check if download & upload directories are not empty
    wf_set_download_state(wu_dir_empty(wc_getFWDownloadFolder()));
    if(!wf_was_download_empty() && !wu_clear_dir(wc_getFWDownloadFolder())) {
        pu_log(LL_ERROR, "Can\'t clear %s : %d %s. Abort.", wc_getFWDownloadFolder(), errno, strerror(errno));
        exit(1);
    }
    wf_set_upgrade_state(wu_dir_empty(wc_getFWUpgradeFolder()));
    if(!wf_was_upgrade_empty() && !wu_clear_dir(wc_getFWUpgradeFolder())) {
        pu_log(LL_ERROR, "Can\'t clear %s : %d %s. Abort.", wc_getFWUpgradeFolder(), errno, strerror(errno));
        exit(1);
    }

//WUD starts - Agent and Proxy according to configuration parameters - binary name and working directorychild processes
    wm_child_descriptor_t* agent_cd = wm_create_cd(wc_getAgentProcessName(), wc_getAgentBinaryName(), wc_getAgentWorkingDirectory(), wc_getAgentRunParameters(), 0);
    wm_child_descriptor_t* proxy_cd = wm_create_cd(wc_getProxyProcessName(), wc_getProxyBinaryName(), wc_getProxyWorkingDirectory(), wc_getProxyRunParameters(), 0);
    wa_start_child(agent_cd);
    wa_start_child(proxy_cd);

    printf("WUD stop\n");

    return 0;
}