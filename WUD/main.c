/*
 *  Copyright 2017 People Power Company
 *
 *  This code was developed with funding from People Power Company
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */
/*
    Created by gsg on 29/11/16.

    Main WUD function. From this place stats the Gateway
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>

#include "pr_ptr_list.h"

#include "wh_manager.h"
#include "wc_defaults.h"
#include "wc_settings.h"
#include "wu_utils.h"
#include "wm_childs_info.h"
#include "wa_manage_children.h"
#include "wf_upgrade.h"
#include "wa_reboot.h"
#include "wt_threads.h"


/****************************************
 * Log the WUD configuration
 */
static void print_WUD_start_params();

/*
 * Debugging utility
 */
volatile uint32_t contextId = 0;
void signalHandler( int signum ) {
    pu_log(LL_ERROR, "WUD.%s: Interrupt signal (%d) received. ContextId=%d thread_id=%d\n", __FUNCTION__, signum, contextId, pthread_self());
    exit(signum);
}

int main(int argc, char* argv[]) {
    signal(SIGSEGV, signalHandler);
    signal(SIGBUS, signalHandler);
    signal(SIGINT, signalHandler);
    signal(SIGFPE, signalHandler);
    signal(SIGKILL, signalHandler);

    char* config = WD_DEFAULT_CFG_FILE_NAME;

    if(argc > 1) {
        int c = getopt(argc, argv, "p:");
        if(c != 'p') {
            fprintf(stderr, "Wrong start parameter. Only -p<config_file_path_and_name> allowed");
            exit(-1);
        }
        config = optarg;
    }

    printf("WUD launcher start\n");
/* read configuration file */
    if(!wc_load_config(config)) {
        fprintf(stderr, "Can\'t read configuration file %s: %d %s. Abort.\n", config, errno, strerror(errno));
        exit(1);
    }
/* initiate logger */
    pu_start_logger(wc_getLogFileName(), wc_getLogRecordsAmt(), wc_getLogVevel());

    if(wc_getWUDDelay()) {
        pu_log(LL_INFO, "WUD start delayed for %d second(s)...", wc_getWUDDelay());
        sleep(wc_getWUDDelay());
    }

/* WUD must check if /var/run/wud.pid exists */
    if(wu_process_exsists(WC_DEFAULT_WUD_NAME)) {
        pu_log(LL_ERROR, "WUD is already running. Abort");
        exit(1);
    }
/* create pid file (var/run/wud.pid */
    if(!wu_create_pid_file(WC_DEFAULT_WUD_NAME, getpid())) {
        pu_log(LL_ERROR, "Can\'t create PID file for %s: %d %s. Abort", WC_DEFAULT_WUD_NAME, errno, strerror(errno));
        exit(1);
    }

/* check if download & upload directories are not empty */
/* NB! Don't change the state even after files deletion: the initial state will be reported to the cloud! */
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

/* WUD starts - Agent and Proxy according to configuration parameters - binary name and working directory child processes */
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

#ifndef WUD_NOT_STARTS_AGENT      /* The agent runs by itself on real gateway. */
  if(!wa_start_child(agent_cd)) {
        pu_log(LL_ERROR, "WUD startup: error. %s process start failed. Reboot.", wc_getAgentProcessName());
        wa_reboot();
    }
    pu_log(LL_INFO, "WUD startup: %s start", wc_getAgentProcessName());
#endif
    if(!wa_start_child(proxy_cd)) {
        pu_log(LL_ERROR, "WUD startup: error. %s process start failed. Reboot.", wc_getProxyProcessName());
        wa_reboot();
    }
    pu_log(LL_INFO, "WUD startup: %s start", wc_getProxyProcessName());
    print_WUD_start_params();

    wh_mgr_init();
    wt_request_processor();
    wh_mgr_destroy();
    wa_reboot();

    printf("WUD stop\n");

    return 0;
}

static void print_WUD_start_params() {
    pu_log(LL_INFO, "WUD start parameters:");
    pu_log(LL_INFO, "\tLog file name: %s", wc_getLogFileName());
    pu_log(LL_INFO, "\t\tRecords amount in log file: %d", wc_getLogRecordsAmt());
    pu_log(LL_INFO, "\t\tLog level: %d", wc_getLogVevel());
    pu_log(LL_INFO, "\tReboot if requested: %d", wc_getRebootByRequest());
    pu_log(LL_INFO, "\tWUD working directory: %s", wc_getWorkingDir());
    pu_log(LL_INFO, "\tWUD communication port: %d", wc_getWUDPort());
    pu_log(LL_INFO, "\tWUD delay before startup in seconds:%d", wc_getWUDDelay());
    pu_log(LL_INFO, "\tWUD delay on Agent&Proxy graceful shutdoun: %d", wc_getChildrenShutdownTO());
    pu_log(LL_INFO, "\tCurlopt SSL Verify Peer: %d", wc_getCurloptSSLVerifyPeer());
    pu_log(LL_INFO, "\tCurlopt CA Info: %s", wc_getCurloptCAInfo());

    pu_log(LL_INFO, "Agent settings:");
    pu_log(LL_INFO, "\tAgent name: %s", wc_getAgentProcessName());
    pu_log(LL_INFO, "\tAgent executable file name: %s", wc_getAgentBinaryName());
    pu_log(LL_INFO, "\tAgent working directory: %s", wc_getAgentWorkingDirectory());
    pu_log(LL_INFO, "\tAgent watchdog timeout in seconds: %d", wc_getAgentWDTimeoutSec());

    pu_log(LL_INFO, "Proxy settings:");
    pu_log(LL_INFO, "\tProxy name: %s", wc_getProxyProcessName());
    pu_log(LL_INFO, "\tProxy executable file name: %s", wc_getProxyBinaryName());
    pu_log(LL_INFO, "\tProxy working directory: %s", wc_getProxyWorkingDirectory());
    pu_log(LL_INFO, "\tProxy watchdog timeout in seconds: %d", wc_getProxyWDTimeoutSec());
}