//
// Created by gsg on 29/10/16.
//

#include <stdio.h>
#include <stdlib.h>

#include "presto_release_version.h"

#include "pc_cli.h"
#include "pc_settings.h"
#include "pc_defaults.h"
#include "pt_threads.h"
#include "ph_manager.h"
#include "pf_cloud_conn_params.h"

static void print_Proxy_start_params();

int main(int argc, char* argv[]) {

    printf("Presto v %s\n", PRESTO_FIRMWARE_VERSION);
    
    if(argc > 1) {
        if(!pc_cli_process_params(argc, argv)) exit(0); //Run from command line
    }
    else {
        if(!pc_load_config(DEFAULT_CFG_FILE_NAME)) exit(-1);    //Run w/o input parameters
    }
    pc_readFWVersion(); //Get the current FW version from file DEFAULT_FW_VERSION_FILE

    pu_start_logger(pc_getLogFileName(), pc_getLogRecordsAmt(), pc_getLogVevel());

    print_Proxy_start_params();

    if(!pf_get_cloud_conn_params()) exit(-1);

    ph_mgr_start();

////////
    pt_main_thread();

//cURL stop
    ph_mgr_stop();
//Logger stop
    pu_stop_logger();
    exit(0);
}

static void print_Proxy_start_params() {
    char buf[500];
    pu_log(LL_INFO, "Proxy start parameters:");
    pu_log(LL_INFO, "\tLog file name: %s", pc_getLogFileName());
    pu_log(LL_INFO, "\t\tRecords amount in log file: %d", pc_getLogRecordsAmt());
    pu_log(LL_INFO, "\t\tLog level: %d", pc_getLogVevel());
    pu_log(LL_INFO, "\tProxy-Agent communication port: %d", pc_getAgentPort());
    pu_log(LL_INFO, "\tProxy-WUD communication port: %d", pc_getWUDPort());
    pu_log(LL_INFO, "\tProxy name: %s", pc_getProxyName());
    pu_log(LL_INFO, "\tProxy watchdog sendind interval in seconds: %d", pc_getProxyWDTO());
    pc_getMainCloudURL(buf, sizeof(buf));
    pu_log(LL_INFO, "\tMain cloud URL: %s", buf);
    pu_log(LL_INFO, "\tRequest for update the Contact URL interval in hours: %d", pc_getCloudURLTOHrs());
    pu_log(LL_INFO, "\tFirmware wersion info to Cloud interval in hours: %d", pc_getFWVerSendToHrs());
}