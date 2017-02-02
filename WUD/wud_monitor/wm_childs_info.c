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

static wm_child_descriptor_t child_array[PR_CHILD_SIZE] = {0};

//Return idx if OK, PR_CHILD_SIZE if error
pr_child_t  wm_create_cd(const char* pn, const char* bn, const char* wd, char* const* sp, unsigned int wd_to, pid_t pid) {
    assert(pn);
    assert(bn);
    for(unsigned int i = 0; i < PR_CHILD_SIZE; i++) {
        if (child_array[i].process_name) continue;
        child_array[i].pid = pid;
        child_array[i].working_directory = (wd) ? strdup(wd) : strdup("./");
        child_array[i].start_parameters = pr_duplicate_ptr_list(child_array[i].start_parameters, sp);
        child_array[i].binary_name = strdup(bn);
        child_array[i].process_name = strdup(pn);
        pr_store_child_name(i, pn);                     // Not nice, but the library level hasn't the access to configuration :-(
        child_array[i].watchdog_to = wd_to;
        return (pr_child_t) i;
    }
    return PR_CHILD_SIZE;
}
pid_t wm_child_get_pid(pr_child_t idx) { // return pid or 0
    if(!pr_child_t_range_check(idx)) return 0;
    return child_array[idx].pid;
}
int wm_child_set_pid(pr_child_t idx, pid_t pid) {
    if(!pr_child_t_range_check(idx)) return 0;
    child_array[idx].pid = pid;
    return 1;
}
const char* wm_child_get_binary_name(pr_child_t idx) {
    if(!pr_child_t_range_check(idx)) return NULL;
    return child_array[idx].binary_name;
}
char** const wm_child_get_start_parameters(pr_child_t idx) {
    if(!pr_child_t_range_check(idx)) return NULL;
    return child_array[idx].start_parameters;
}
const char* wm_child_get_working_directory(pr_child_t idx) {
    if(!pr_child_t_range_check(idx)) return NULL;
    return child_array[idx].working_directory;
}
unsigned int wm_child_get_child_to(pr_child_t idx) {
    if(!pr_child_t_range_check(idx)) return 0;
    return child_array[idx].watchdog_to;
}
const char* wm_get_child_name(pr_child_t idx) {
    if(!pr_child_t_range_check(idx)) return NULL;
    return child_array[idx].process_name;
}
pr_child_t wm_get_child_descr(const char* name) {
    for(unsigned int i = 0; i < PR_CHILD_SIZE; i++) {
        if(strcmp(name, child_array[i].process_name)== 0) return i;
    }
    return PR_CHILD_SIZE;
}
