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
