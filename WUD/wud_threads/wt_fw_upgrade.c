//
// Created by gsg on 07/12/16.
//
#include <pthread.h>
#include <string.h>
#include <malloc.h>
#include <errno.h>

#include "pr_commands.h"
#include "lib_sha_256.h"

#include "wc_defaults.h"
#include "wc_settings.h"
#include "wt_queues.h"
#include "wu_utils.h"
#include "wh_manager.h"
#include "wf_upgrade.h"

#include "wt_fw_upgrade.h"


#define PT_THREAD_NAME "FW_UPGRADE"
/////////////////////////////
//Notification messages

////////////////////////////
static pthread_t id;
static pthread_attr_t attr;
static volatile int fwu_stop;

static pu_queue_t* to_main;       //notifications to request processor
static pu_queue_t* to_cloud;

static char* connection_string = NULL;
static char* file_name = NULL;
static char* check_sum = NULL;

static char alert[LIB_HTTP_MAX_MSG_SIZE];    //will be sent to RP. RP should forward it to the cloud
static char deviceID[LIB_HTTP_DEVICE_ID_SIZE];
static char fw_version[LIB_HTTP_FW_VERSION_SIZE];

static void* read_proc(void* params);
typedef enum {FWU_NOTIFY_START, FWU_NOTIFY_FAIL, FWU_NOTIFY_OK} fw_notify_t;
static void notify(fw_notify_t n);


int wt_start_fw_upgrade(pr_cmd_fwu_start_t fwu_start) {
    connection_string = strdup(fwu_start.file_server_url);
    file_name = strdup(fwu_start.file_name);
    check_sum = strdup(fwu_start.check_sum);

    if(pthread_attr_init(&attr)) goto on_error;
    if(pthread_create(&id, &attr, &read_proc, NULL)) goto on_error;
    return 1;
on_error:
    free(connection_string);
    free(file_name);
    free(check_sum);
    return 0;
}
void wt_stop_fw_upgrade() {
    void *ret;

    wt_set_stop_fw_upgrade();
    pthread_join(id, &ret);
    pthread_attr_destroy(&attr);
    if(connection_string) free(connection_string);
    if(file_name) free(file_name);
    if(check_sum) free(check_sum);
}
void wt_set_stop_fw_upgrade() {
    fwu_stop = 1;
}

static void* read_proc(void* params) {
    fwu_stop = 0;

    char full_path_n_file[LIB_HTTP_MAX_URL_SIZE];

    wc_getDeviceID(deviceID, sizeof(deviceID));
    wc_getFWVersion(fw_version, sizeof(fw_version));
    wu_create_file_name(full_path_n_file, sizeof(full_path_n_file), wc_getFWDownloadFolder(), file_name, "");

    to_main = wt_get_gueue(WT_to_Main);
    to_cloud = wt_get_gueue(WT_to_Cloud);
//Declares the start of proces
    notify(FWU_NOTIFY_START);

    if(!wh_read_file(full_path_n_file, connection_string, LIB_HTTP_MAX_FGET_RETRIES)) {      //Download fails
        pu_log(LL_INFO, "%s: FW Upgrade file(s) download fails.", PT_THREAD_NAME);
        notify(FWU_NOTIFY_FAIL);
        goto on_exit;
    }
//We get the file(s)!
    pu_log(LL_INFO, "%s: FW Upgrade file(s) download finished.", PT_THREAD_NAME);
    wf_set_download_state(WF_STATE_BUZY);

//Check files
    if(!wf_check_file(check_sum, wc_getFWDownloadFolder(), file_name)) {
        pu_log(LL_ERROR, "%s: FW Upgrade file check sum checking failed", PT_THREAD_NAME);
        notify(FWU_NOTIFY_FAIL);
        goto on_exit;
    }
//Fies are OK. Too good to be truth...
    pu_log(LL_INFO, "%s: FW Upgrade file check sum verified OK.", PT_THREAD_NAME);

//Move the file to upgrade dir and rename it
    if(!wu_move_n_rename(wc_getFWDownloadFolder(), file_name, wc_getFWUpgradeFolder(), WC_FWU_FILE_NAME)) {  // Huston, we got a problem!
        notify(FWU_NOTIFY_FAIL);
        goto on_exit;
    }
//All done
    wf_set_upgrade_state(WF_STATE_BUZY);
    notify(FWU_NOTIFY_OK);

//Wait until WUD command
    while(!fwu_stop) sleep(1);
on_exit:
    pu_log(LL_INFO, "%s is finished", PT_THREAD_NAME);
    pthread_exit(NULL);
}
//FWU_NOTIFY_START FWU_NOTIFY_FAIL FWU_NOTIFY_OK
static void notify(fw_notify_t n) {
    switch(n) {
        case FWU_NOTIFY_START:
            pr_make_fw_status4cloud(alert, sizeof(alert), PR_FWU_STATUS_START, fw_version, deviceID);
            pu_queue_push(to_cloud, alert, strlen(alert)+1);
            break;
        case FWU_NOTIFY_FAIL:
//Notify cloud
            pr_make_fw_status4cloud(alert, sizeof(alert), PR_FWU_STATUS_STOP, fw_version, deviceID);
            pu_queue_push(to_cloud, alert, strlen(alert)+1);
//Notify WUD
            pr_make_fw_fail4WUD(alert, sizeof(alert), deviceID);
            pu_queue_push(to_main, alert, strlen(alert)+1);
            break;
        case FWU_NOTIFY_OK:
//Notify cloud - "in process" - ready for reboot
            pr_make_fw_status4cloud(alert, sizeof(alert), PR_FWU_STATUS_PROCESS, fw_version, deviceID);
            pu_queue_push(to_cloud, alert, strlen(alert)+1);
//Notify WUD
            pr_make_fw_ok4WUD(alert, sizeof(alert), deviceID);
            pu_queue_push(to_main, alert, strlen(alert)+1);
            break;
        default:
            break;
    }
}