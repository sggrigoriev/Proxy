//
// Created by gsg on 01/12/16.
//

#include <pu_logger.h>
#include <wu_utils.h>
#include <wa_reboot.h>
#include "wf_upgrade.h"


static int download_empty;
static int upgrade_empty;


void wf_set_download_state(int empty) {
    download_empty = empty;
}
void wf_set_upgrade_state(int empty) {
    upgrade_empty= empty;
}

int wf_was_download_empty() {
    return download_empty;
}
int wf_was_upgrade_empty() {
    return upgrade_empty;
}
//TODO: implement files check function!
int wf_check_files(const char* path) {
    char** flist = wu_get_flist(path);
    if(!flist) {
        pu_log(LL_ERROR, "wf_check_files: Memory allocation error. Reboot");
        wa_reboot();
        return 0;         //Just for pro-forma...
    }
    unsigned len = 0;
    while(flist[len]) {
        pu_log(LL_ERROR, "I'm wf_check_files(). Please born me! File name = %s", flist[len]);
        len++;
    }
    wu_free_flist(flist);

    return 1;
}