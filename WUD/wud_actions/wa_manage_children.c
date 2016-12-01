//
// Created by gsg on 30/11/16.
//
#include <signal.h>
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <wm_childs_info.h>

#include "wc_settings.h"
#include "wu_utils.h"
#include "wm_childs_info.h"
#include "wa_manage_children.h"

//return 1 if ok, 0 if not
void wa_start_child(wm_child_descriptor_t* descr) {
    char** buf = NULL;

    buf = wu_prepare_papams_list(buf, descr->binary_name, descr->start_parameters);
    unsigned int i = 0;
    printf("%s start params\n", descr->process_name);
    while(buf[i]) { printf("param[%d] = %s\n", i, buf[i]); i++;}
    descr->pid = wu_start_process(descr->binary_name, buf, descr->working_directory);
    wu_delete_params_list(buf);
}
void wa_stop_child(wm_child_descriptor_t* descr) {
    assert(descr);
    kill(descr->pid, SIGTERM);
    sleep(wc_getChildrenShutdownTO());
    kill(descr->pid, SIGKILL);  //In case the process didn't listen SIGTERM carefully...
}

void wa_restart(wm_child_descriptor_t* descr) {
    wa_stop_child(descr);
    wa_start_child(descr);
}
