//
// Created by gsg on 30/11/16.
//
// Contains Proxy & Agent info and access functions

#ifndef PRESTO_WM_CHILDS_INFO_H
#define PRESTO_WM_CHILDS_INFO_H

#include <sys/types.h>

typedef struct {
    char* process_name;
    char* binary_name;
    char* working_directory;
    char** start_parameters;
    pid_t pid;
} wm_child_descriptor_t;

wm_child_descriptor_t* wm_create_cd(const char* pn, const char* bn, const char* wd, char* const* sp, pid_t pid); //Return cd if OK, NULL if error
void wm_delete_cd(wm_child_descriptor_t* cd);


#endif //PRESTO_WM_CHILDS_INFO_H
