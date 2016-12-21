//
// Created by gsg on 07/12/16.
//
#include <pthread.h>
#include <string.h>
#include <malloc.h>
#include <wf_upgrade.h>
#include <wu_utils.h>
#include <errno.h>
#include <pr_commands.h>

#include "wc_defaults.h"
#include "wc_settings.h"
#include "wt_queues.h"
#include "wt_http_utl.h"

#include "wt_fw_upgrade.h"


#define PT_THREAD_NAME "FW_UPGRADE"
/////////////////////////////
//Notification messages

////////////////////////////
static pthread_t id;
static pthread_attr_t attr;
static volatile int stop;

static pu_queue_t* to_rp;       //notifications to request processor

static char* connection_string;

static void* read_proc(void* params);

int wt_start_fw_upgrade(const char* conn_string) {
    if(!conn_string) return 0;
    if(pthread_attr_init(&attr)) return 0;
    if(pthread_create(&id, &attr, &read_proc, NULL)) return 0;
    connection_string = strdup(conn_string);
    return 1;
}
void wt_stop_fw_upgrade() {
    void *ret;

    wt_set_stop_fw_upgrade();
    pthread_join(id, &ret);
    pthread_attr_destroy(&attr);
}
void wt_set_stop_fw_upgrade() {
    stop = 1;
}

//TODO: PR_FW_UPGRADE_CANCEL - use this if wana abort the process!!!
//TODO: add a reaction to the possible stop = 1!!! (add as a parameter for cURL callbacks
static void* read_proc(void* params) {
    stop = 0;
    char* alert;    //will be sent to RP. RP should forward it to the cloud
    to_rp = wt_get_gueue(WT_FWInfo);

    if(!wt_http_gf_init()) {
        pu_log(LL_ERROR, "%s: Get file initiation failed. Stop.", PT_THREAD_NAME);
        pr_make_fwu_alert(&alert, PR_FWU_INIT_FAILED, WC_DEFAULT_WUD_NAME);
        pu_queue_push(to_rp, alert, strlen(alert)+1);
        free(alert);
        wt_set_stop_fw_upgrade();
    }
    pr_make_fwu_alert(&alert, PR_FWU_INIT_OK, WC_DEFAULT_WUD_NAME);
    pu_queue_push(to_rp, alert, strlen(alert)+1);
    free(alert);

   if(wt_http_get_file(connection_string, wc_getFWDownloadFolder())) {                      //We get the file(s)!
       pu_log(LL_INFO, "%s: FW Upgrade file(s) download finished.", PT_THREAD_NAME);
       wf_set_download_state(WF_STATE_BUZY);
       pr_make_fwu_alert(&alert, PR_FWU_DOWNLOAD_OK, WC_DEFAULT_WUD_NAME);
       pu_queue_push(to_rp, alert, strlen(alert)+1);
       free(alert);
//Check files
       if(wf_check_files(wc_getFWDownloadFolder())) {      //Fies are OK
           pu_log(LL_INFO, "%s: FW Upgrade file(s) verified.", PT_THREAD_NAME);
           pr_make_fwu_alert(&alert, PR_FWU_CHECK_OK, WC_DEFAULT_WUD_NAME);
           pu_queue_push(to_rp, alert, strlen(alert)+1);
           free(alert);
           if(wu_move_files(wc_getFWUpgradeFolder(), wc_getFWDownloadFolder())) {  // Files moved to Upgrade dir!
               wf_set_upgrade_state(WF_STATE_BUZY);
               pu_log(LL_INFO, "%s: FW Upgrade file(s) moved to %s.", PT_THREAD_NAME, wc_getFWDownloadFolder());
               pr_make_fwu_alert(&alert, PR_FWU_MOVE_OK, WC_DEFAULT_WUD_NAME);
               pu_queue_push(to_rp, alert, strlen(alert)+1);
               free(alert);
           }
           else {
               pu_log(LL_ERROR, "%s: FW Upgrade file(s) move to %s failed: %d %s", PT_THREAD_NAME, wc_getFWDownloadFolder(), errno, strerror(errno));
               pr_make_fwu_alert(&alert, PR_FWU_MOVE_FAIL, WC_DEFAULT_WUD_NAME);
               pu_queue_push(to_rp, alert, strlen(alert)+1);
               free(alert);
           }
       }
       else {  //Check fais
           pu_log(LL_INFO, "%s: FW Upgrade file(s) check failed.", PT_THREAD_NAME);
           pr_make_fwu_alert(&alert, PR_FWU_CHECK_FAIL, WC_DEFAULT_WUD_NAME);
           pu_queue_push(to_rp, alert, strlen(alert)+1);
           free(alert);
       }
   }
   else {  //Download fails
       pu_log(LL_INFO, "%s: FW Upgrade file(s) download fails.", PT_THREAD_NAME);
       pr_make_fwu_alert(&alert, PR_FWU_DOWNLOAD_FAIL, WC_DEFAULT_WUD_NAME);
       pu_queue_push(to_rp, alert, strlen(alert)+1);
       free(alert);

   }

    free(connection_string);
    wt_http_gf_destroy();
    pu_log(LL_INFO, "%s is finished", PT_THREAD_NAME);
    pthread_exit(NULL);
}
