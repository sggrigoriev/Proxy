//
// Created by gsg on 30/11/16.
//
#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include <wm_childs_info.h>
#include <pr_ptr_list.h>

#include "wc_settings.h"
#include "wu_utils.h"
#include "wa_manage_children.h"

//return 1 if ok, 0 if not
int wa_start_child(wa_child_t id) {
    char** buf = NULL;

    buf = pr_push(buf, pr_child_get_binary_name(id), pr_child_get_start_parameters(id));
    unsigned int i = 0;

    pu_log(LL_DEBUG,"%s start params", pr_chld_2_string(id));
    while(buf[i]) { pu_log(LL_DEBUG," param[%d] = %s", i, buf[i]); i++;}

    int ret = pr_child_set_pid(id, wu_start_process(pr_child_get_binary_name(id), buf, pr_child_get_working_directory(id)));
    pt_delete_ptr_list(buf);
    return ret;
}
void wa_stop_child(wa_child_t id) {
    kill(pr_child_get_pid(id), SIGTERM);
    sleep(wc_getChildrenShutdownTO());
    kill(pr_child_get_pid(id), SIGKILL);  //In case the process didn't listen SIGTERM carefully...
}

int wa_restart_child(wa_child_t id) {
    wa_stop_child(id);
    return wa_start_child(id);
}
