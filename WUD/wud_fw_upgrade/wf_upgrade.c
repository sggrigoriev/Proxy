//
// Created by gsg on 01/12/16.
//
#include <string.h>
#include <errno.h>

#include "pu_logger.h"
#include "lib_sha_256.h"

#include "wc_defaults.h"
#include "wu_utils.h"
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
int wf_check_file(const char* check_sum, const char* path, const char* file_name) {
    char full_fname[WC_MAX_PATH];
    wu_create_file_name(full_fname, sizeof(full_fname), path, file_name, "");

    if(strlen(check_sum) != LIB_SHA256_BLOCK_SIZE) {
        pu_log(LL_ERROR, "wf_check_file: incorrect check sum sent by the cloud: %d bytes instead of %d", strlen(check_sum), LIB_SHA256_BLOCK_SIZE);
        return 0;
    }
    FILE* f = fopen(full_fname, "r");
    if(!f) {
        pu_log(LL_ERROR, "wf_check_file: error opening %s: %d, %s", full_fname, errno, strerror(errno));
        return 0;
    }
    int ret = lib_sha_file_compare(check_sum, LIB_SHA256_BLOCK_SIZE, f);
    fclose(f);
    return ret;
}