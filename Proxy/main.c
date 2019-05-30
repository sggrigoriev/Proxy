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
    Created by gsg on 29/10/16.

    Proxy process main function
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <pthread.h>
#include <signal.h>
#include <pr_ptr_list.h>

#include "presto_release_version.h"

#include "pc_config.h"
#include "pc_cli.h"
#include "pc_settings.h"
#include "pc_defaults.h"
#include "pt_threads.h"
#include "ph_manager.h"
#include "pf_cloud_conn_params.h"

/* Help for Proxy start parameters syntax */
static void print_Proxy_start_params();

/*
 * Debugging utility
 */
volatile uint32_t contextId = 0;
void signalHandler( int signum ) {
    pu_log(LL_ERROR, "PROXY.%s: Interrupt signal (%d) received. ContextId=%d thread_id=%lu\n", __FUNCTION__, signum, contextId, pthread_self());
    fprintf(stdout, "PROXY.%s: Interrupt signal (%d) received. ContextId=%d thread_id=%lu\n", __FUNCTION__, signum, contextId, pthread_self());
    exit(signum);
}

/* Main function */
int main(int argc, char* argv[]) {
    signal(SIGSEGV, signalHandler);
    signal(SIGBUS, signalHandler);
    signal(SIGINT, signalHandler);
    signal(SIGFPE, signalHandler);
    signal(SIGKILL, signalHandler);

    if(argc > 1) {
        pu_set_log_level(LL_SILENT);
        pc_cli_params_t par = pc_cli_process_params(argc, argv);

        if(par.action == PCLI_VERSION) {
            pc_cli_printVersion();
            exit(0);
        }

        if(par.parameter) {
            if(!pc_load_config(par.parameter)) {
                fprintf(stdout, "%s: Error configuration %s load. Proxy exiting\n", __FUNCTION__, par.parameter);
                exit(-1);
            }
        }
        else {
            if(!pc_load_config(DEFAULT_CFG_FILE_NAME)) {
                fprintf(stdout, "%s: Error configuration %s load. Proxy exiting\n", __FUNCTION__, DEFAULT_CFG_FILE_NAME);
                exit(-1);
            }
        }
        switch(par.action) {
                case PCLI_VERSION:
                    pc_cli_printVersion();
                    exit(0);
                case PCLI_DEVICE_ID:
                    pc_cli_printDeviceID();
                    exit(0);
                default:
                    if(!par.parameter) {
                        pc_cli_printUsage();
                        exit(0);
                    }
                    break;
        }
     }
    else {
        if(!pc_load_config(DEFAULT_CFG_FILE_NAME)) exit(-1);    /* Run w/o input parameters */
    }
    
    printf("Presto v %s\n", PRESTO_FIRMWARE_VERSION);
    
    pc_readFWVersion(); /* Get the current FW version from file DEFAULT_FW_VERSION_FILE */

    pu_start_logger(pc_getLogFileName(), pc_getLogRecordsAmt(), pc_getLogVevel());

    print_Proxy_start_params();

    if(!pf_get_cloud_conn_params()) {
        fprintf(stdout, "%s: Can't get cloud connection parameters. Proxy exiting\n", __FUNCTION__);
        exit(-1);
    }

    ph_mgr_init();          /* Initiates PH manager, no cloud connections are made */

    pt_main_thread();

/* HTTP mgr stop */
    ph_mgr_destroy();       /* Close all existing connections, deinit PH manager */
/* Logger stop */
    pu_stop_logger();
    fprintf(stdout, "Proxy exiting]n");
    exit(0);
}

static void print_Proxy_start_params() {
    char buf[1024]={0};
    pu_log(LL_INFO, "\n%s", get_version_printout(PRESTO_FIRMWARE_VERSION, buf, sizeof(buf)));
    pu_log(LL_INFO, "Proxy start parameters:");
    pu_log(LL_INFO, "\tLog file name: %s", pc_getLogFileName());
    pu_log(LL_INFO, "\t\tRecords amount in log file: %d", pc_getLogRecordsAmt());
    pu_log(LL_INFO, "\t\tLog level: %s - %d", getLogLevel(pc_getLogVevel()), pc_getLogVevel());
    pu_log(LL_INFO, "\tProxy-Agent communication port: %d", pc_getAgentPort());
    pu_log(LL_INFO, "\tProxy-WUD communication port: %d", pc_getWUDPort());
    pu_log(LL_INFO, "\tProxy name: %s", pc_getProxyName());
    pu_log(LL_INFO, "\tProxy watchdog sending interval in seconds: %d", pc_getProxyWDTO());

    pc_getMainCloudURL(buf, sizeof(buf));
    pu_log(LL_INFO, "\tMain cloud URL: %s", buf);

    pu_log(LL_INFO, "\tRequest for update the Contact URL interval in hours: %d", pc_getCloudURLTOHrs());
    pu_log(LL_INFO, "\tFirmware version info to Cloud interval in hours: %d", pc_getFWVerSendToHrs());
    pu_log(LL_INFO, "\tSSL request for contact URL request: %d", pc_getSetSSLForCloudURLRequest());

    pu_log(LL_INFO, "\tCurlopt SSP Verify Peer: %d", pc_getCurloptSSPVerifyPeer());
    pu_log(LL_INFO, "\tCurlopt CA Info: %s", pc_getCurloptCAInfo());

    pu_log(LL_INFO, "\tProxy device ID preffix: %s", pc_getProxyDeviceIDPrefix());
    pu_log(LL_INFO, "\tReboot if Cloud rejects Proxy: %d", pc_rebootIfCloudRejects());

    pu_log(LL_INFO, "\tCloud POST connection timeout sec: %d", pc_getCloudConnTimeout());
    pu_log(LL_INFO, "\tPost attempts amount: %d", pc_getCloudPostAttempts());
    pu_log(LL_INFO, "\tLong GET keepalive interval sec: %d", pc_getLongGetKeepaliveTO());
    pu_log(LL_INFO, "\tLong GET timeout sec: %d", pc_getLongGetTO());

    pu_log(LL_INFO, "\tAllowed domais list: %s", pr_ptr_list2string(buf, sizeof(buf), pc_getAllowedDomainsList()));
}