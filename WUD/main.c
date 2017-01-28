//
// Created by gsg on 29/11/16.
//

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <wm_childs_info.h>
#include <wa_manage_children.h>
#include <wf_upgrade.h>
#include <wa_reboot.h>
#include <wt_threads.h>
#include <pr_ptr_list.h>

#include "wc_defaults.h"
#include "wc_settings.h"
#include "wu_utils.h"

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
    pu_start_logger(wc_getLogFileName(), wc_getLogRecordsAmt(), wc_getLogVevel());

//check if download & upload directories are not empty
//NB! Don't change the state even after files deletion: the initial state will be reported to the cloud!
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

//WUD starts - Agent and Proxy according to configuration parameters - binary name and working directory child processes
    char buf[500];
    pu_log(LL_DEBUG, "WUD: Agent descriptor entries: process name = %s, binary name = %s, working directory = %s, run_parameters = %s, wd to = %d",
           wc_getAgentProcessName(), wc_getAgentBinaryName(), wc_getAgentWorkingDirectory(),
           pr_ptr_list2string(buf, sizeof(buf), wc_getAgentRunParameters()), wc_getAgentWDTimeoutSec());
    pr_child_t agent_cd = wm_create_cd(
                                        wc_getAgentProcessName(),
                                        wc_getAgentBinaryName(),
                                        wc_getAgentWorkingDirectory(),
                                        wc_getAgentRunParameters(),
                                        wc_getAgentWDTimeoutSec(),
                                        0
                                    );
    pu_log(LL_DEBUG, "WUD: Proxy descriptor entries: process name = %s, binary name = %s, working directory = %s, run_parameters = %s, wd to = %d",
           wc_getProxyProcessName(), wc_getProxyBinaryName(), wc_getProxyWorkingDirectory(),
           pr_ptr_list2string(buf, sizeof(buf), wc_getProxyRunParameters()), wc_getProxyWDTimeoutSec());
    pr_child_t proxy_cd = wm_create_cd(
                                        wc_getProxyProcessName(),
                                        wc_getProxyBinaryName(),
                                        wc_getProxyWorkingDirectory(),
                                        wc_getProxyRunParameters(),
                                        wc_getProxyWDTimeoutSec(),
                                        0
                                    );

    if(!wa_start_child(agent_cd)) {
        pu_log(LL_ERROR, "WUD startup: error. %s process start failed. Reboot.", wc_getAgentProcessName());
        wa_reboot();
    }
    pu_log(LL_INFO, "WUD startup: %s start", wc_getAgentProcessName());
    if(!wa_start_child(proxy_cd)) {
        pu_log(LL_ERROR, "WUD startup: error. %s process start failed. Reboot.", wc_getProxyProcessName());
        wa_reboot();
    }
    pu_log(LL_INFO, "WUD startup: %s start", wc_getProxyProcessName());

    if(!wt_request_processor()) {
        pu_log(LL_ERROR, "WUD startup: request procesor failed. Reboot");
    }
    else {
        pu_log(LL_WARNING, "WUD startup: request procesor finished. Reboot");
    }
//If we're here - something really wrong - reboot this zoo with sluts and blackjack!
    wa_reboot();

    printf("WUD stop\n");

    return 0;
}