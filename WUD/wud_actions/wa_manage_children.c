/*
 *  Copyright 2017 People Power Company
 *
 *  This code was developed with funding from People Power Company
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */
/*
    Created by gsg on 30/11/16.
*/
#include <signal.h>
#include <unistd.h>

#include "pr_ptr_list.h"

#include "wm_childs_info.h"
#include "wc_settings.h"
#include "wu_utils.h"
#include "wa_alarm.h"

#include "wa_manage_children.h"

int wa_start_child(pr_child_t id) {
    char** buf = NULL;

    buf = pr_push(buf, wm_child_get_binary_name(id), wm_child_get_start_parameters(id));
    unsigned int i = 0;

    pu_log(LL_DEBUG,"%s start params", pr_chld_2_string(id));
    while(buf[i]) { pu_log(LL_DEBUG," param[%d] = %s", i, buf[i]); i++;}

    if( access( wm_child_get_binary_name(id), F_OK ) == -1 ) {
        pu_log(LL_ERROR, "%s file doesn't exist. Abort", wm_child_get_binary_name(id));
        return 0;
    }
    if( access( wm_child_get_binary_name(id), X_OK ) == -1 ) {
        pu_log(LL_ERROR, "No permission granted to execute %s. Abort", wm_child_get_binary_name(id));
        return 0;
    }

    int ret = wm_child_set_pid(id, wu_start_process(wm_child_get_binary_name(id), buf, wm_child_get_working_directory(id)));
    wa_alarm_reset(id);
    pt_delete_ptr_list(buf);
    return ret;
}

void wa_stop_child(pr_child_t id) {     /* Later. currently we can'r do it */
/*
    kill(wm_child_get_pid(id), SIGTERM);
    sleep(wc_getChildrenShutdownTO());
    kill(wm_child_get_pid(id), SIGKILL);  //In case the process didn't listen SIGTERM carefully...
*/
}

void wa_stop_children() {
    int c;
    for(c = 0; c < PR_CHILD_SIZE; c++) wa_stop_child((pr_child_t)c);
}

int wa_restart_child(pr_child_t id) {
/*
    wa_stop_child(id);
    return wa_start_child(id);
*/
    return 0;
}
