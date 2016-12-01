//
// Created by gsg on 30/11/16.
//

#ifndef PRESTO_WA_MANAGE_CHILDREN_H
#define PRESTO_WA_MANAGE_CHILDREN_H

#include <sys/types.h>

void wa_start_child(wm_child_descriptor_t* descr);   //return 1 if ok, 0 if not
void wa_stop_child(wm_child_descriptor_t* descr);
void wa_restart_child(wm_child_descriptor_t* descr);  //return 1 if ok, 0 if not


#endif //PRESTO_WA_MANAGE_CHILDREN_H
