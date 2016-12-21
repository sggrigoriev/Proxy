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

wa_child_t wm_create_cd(const char* pn, const char* bn, const char* wd, char* const* sp, unsigned int wd_to, pid_t pid); //Return cd if OK, NULL if error

pid_t pr_child_get_pid(wa_child_t idx); // return pid or 0
int pr_child_set_pid(wa_child_t idx, pid_t pid);

const char* pr_child_get_binary_name(wa_child_t idx);
char** const pr_child_get_start_parameters(wa_child_t idx);
const char* pr_child_get_working_directory(wa_child_t idx);
unsigned int pr_child_get_child_to(wa_child_t idx);



#endif //PRESTO_WM_CHILDS_INFO_H
