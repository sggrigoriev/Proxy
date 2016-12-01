//
// Created by gsg on 30/11/16.
//
#include <pthread.h>
#include <malloc.h>
#include <string.h>
#include <assert.h>
#include <wu_utils.h>

#include "wm_childs_info.h"

//Local data



//Return cd if OK, NULL if error
wm_child_descriptor_t* wm_create_cd(const char* pn, const char* bn, const char* wd, char* const * sp, pid_t pid) {
    wm_child_descriptor_t* ret;
    assert(pn);
    assert(bn);
    ret = (wm_child_descriptor_t*)malloc(sizeof(wm_child_descriptor_t));
    ret->pid = pid;
    ret->working_directory = (wd)?strdup(wd):NULL;
    ret->start_parameters = wu_duplicate_params_list(ret->start_parameters, sp);
    ret->binary_name = strdup(bn);
    ret->process_name = strdup(pn);
    return ret;
}

void wm_delete_cd(wm_child_descriptor_t* cd) {
    if(!cd) return;
    if(cd->working_directory) free(cd->working_directory);
    wu_delete_params_list(cd->start_parameters);
    if(cd->binary_name) free(cd->binary_name);
    if(cd->process_name) free(cd->process_name);
    free(cd);
}