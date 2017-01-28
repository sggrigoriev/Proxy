//
// Created by gsg on 30/11/16.
//
#include <signal.h>
#include <unistd.h>

#include "wm_childs_info.h"
#include "pr_ptr_list.h"
#include "wc_settings.h"
#include "wu_utils.h"
#include "wa_alarm.h"
#include "wa_manage_children.h"

//return 1 if ok, 0 if not
int wa_start_child(pr_child_t id) {
    char** buf = NULL;

    buf = pr_push(buf, wm_child_get_binary_name(id), wm_child_get_start_parameters(id));
    unsigned int i = 0;

    pu_log(LL_DEBUG,"%s start params", pr_chld_2_string(id));
    while(buf[i]) { pu_log(LL_DEBUG," param[%d] = %s", i, buf[i]); i++;}

    int ret = wm_child_set_pid(id, wu_start_process(wm_child_get_binary_name(id), buf, wm_child_get_working_directory(id)));
    wa_alarm_reset(id);
    pt_delete_ptr_list(buf);
    return ret;
}
void wa_stop_child(pr_child_t id) {
    kill(wm_child_get_pid(id), SIGTERM);
    sleep(wc_getChildrenShutdownTO());
    kill(wm_child_get_pid(id), SIGKILL);  //In case the process didn't listen SIGTERM carefully...
}

int wa_restart_child(pr_child_t id) {
    wa_stop_child(id);
    return wa_start_child(id);
}
