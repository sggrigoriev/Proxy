//
// Created by gsg on 29/10/16.
//

#include <stdio.h>
#include <stdlib.h>

#include "pc_cli.h"
#include "pc_settings.h"
#include "pc_defaults.h"
#include "pu_logger.h"
#include "pt_http_utl.h"
#include "pt_threads.h"
#include "pf_proxy_activation.h"

int main(int argc, char* argv[]) {

    printf("%s", "Hi, I\'m new Presto!\n");
    
#ifdef PROXY_SEPARATE_RUN    
    if(!pc_cli_process_params(argc, argv)) exit(0); //Run from command line
#else
    if(!pc_load_config(DEFAULT_CFG_FILE_NAME)) exit(-1);    //Run w/o input parameters
#endif

//Initiation steps:
// 1. Start logger & cURL
// 2. Get settings
// 2.1 Get configuration parameters
// 2.2 Get argv parameters & override config parameters in memory
// 2.3 Get device EUI if not set in configuration
// 2.4 Seth the signe "activate" to 0 or 1 if activation needed
// 3. Activation step- do be discussed
// 4. Get permanent URL for work - NB! each time!

    

    pu_start_logger(pc_getLogFileName(), pc_getLogRecordsAmt(), pc_getLogVevel());

    if(!pt_http_curl_init()) exit(-1);

    if(!pf_proxy_activation()) exit(-1);


//Stop everything until the recurring work starts
    pt_http_curl_stop();
//Logger stop
    pu_stop_logger();

    if(!pt_main_thread_start()) exit(-1);
    exit(0);
}
