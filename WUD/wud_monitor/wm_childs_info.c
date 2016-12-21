//
// Created by gsg on 30/11/16.
//
#include <pthread.h>
#include <malloc.h>
#include <string.h>
#include <assert.h>
#include <wu_utils.h>

#include "pr_commands.h"
#include "pr_ptr_list.h"
#include "wm_childs_info.h"

//Local data

static wm_child_descriptor_t child_array[WA_SIZE] = {0};

//Return cd if OK, NULL if error
wa_child_t  wm_create_cd(const char* pn, const char* bn, const char* wd, char* const* sp, unsigned int wd_to, pid_t pid) {
    assert(pn);
    assert(bn);
    for(unsigned int i = 0; i < WA_SIZE; i++) {
        if (child_array[i].process_name) continue;
        child_array[i].pid = pid;
        child_array[i].working_directory = (wd) ? strdup(wd) : strdup("./");
        child_array[i].start_parameters = pr_duplicate_ptr_list(child_array[i].start_parameters, sp);
        child_array[i].binary_name = strdup(bn);
        child_array[i].process_name = strdup(pn);
        pr_store_child_name(i, pn);                     // Not nice, but the library level hasn't the access to configuration :-(
        child_array[i].watchdog_to = wd_to;
        return (wa_child_t) i;
    }
    return WA_SIZE;
}
pid_t pr_child_get_pid(wa_child_t idx) { // return pid or 0
    if(!wa_child_t_range_check(idx)) return 0;
    return child_array[idx].pid;
}
int pr_child_set_pid(wa_child_t idx, pid_t pid) {
    if(!wa_child_t_range_check(idx)) return 0;
    child_array[idx].pid = pid;
    return 1;
}
const char* pr_child_get_binary_name(wa_child_t idx) {
    if(!wa_child_t_range_check(idx)) return NULL;
    return child_array[idx].binary_name;
}
char** const pr_child_get_start_parameters(wa_child_t idx) {
    if(!wa_child_t_range_check(idx)) return NULL;
    return child_array[idx].start_parameters;
}
const char* pr_child_get_working_directory(wa_child_t idx) {
    if(!wa_child_t_range_check(idx)) return NULL;
    return child_array[idx].working_directory;
}
unsigned int pr_child_get_child_to(wa_child_t idx) {
    if(!wa_child_t_range_check(idx)) return 0;
    return child_array[idx].watchdog_to;
}
