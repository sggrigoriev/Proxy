//
// Created by gsg on 30/11/16.
//
// Contains Proxy & Agent info and access functions

#ifndef PRESTO_WM_CHILDS_INFO_H
#define PRESTO_WM_CHILDS_INFO_H

#include <sys/types.h>

#include "pr_commands.h"

typedef struct {
    char* process_name;
    char* binary_name;
    char* working_directory;
    char** start_parameters;
    unsigned int watchdog_to;
    pid_t pid;
} wm_child_descriptor_t;

// Child descriptors just updates their PIDs during the start, so created in WUD startup they'd never been deleted!

pr_child_t wm_create_cd(const char* pn, const char* bn, const char* wd, char* const* sp, unsigned int wd_to, pid_t pid); //Return cd if OK, NULL if error

pid_t wm_child_get_pid(pr_child_t idx); // return pid or 0
int wm_child_set_pid(pr_child_t idx, pid_t pid);

const char* wm_child_get_binary_name(pr_child_t idx);
char** const wm_child_get_start_parameters(pr_child_t idx);
const char* wm_child_get_working_directory(pr_child_t idx);
unsigned int wm_child_get_child_to(pr_child_t idx);
const char* wm_get_child_name(pr_child_t idx);
//return idx or PR_CHILD_SIZE
pr_child_t wm_get_child_descr(const char* name);



#endif //PRESTO_WM_CHILDS_INFO_H
