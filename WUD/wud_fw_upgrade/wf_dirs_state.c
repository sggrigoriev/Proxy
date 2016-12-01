//
// Created by gsg on 30/11/16.
//

#include "wf_dirs_state.h"

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