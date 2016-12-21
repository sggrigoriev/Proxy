//
// Created by gsg on 30/11/16.
//

#ifndef PRESTO_WA_MANAGE_CHILDREN_H
#define PRESTO_WA_MANAGE_CHILDREN_H


#include <pr_commands.h>

int wa_start_child(wa_child_t id);   //return 1 if ok, 0 if not
int wa_restart_child(wa_child_t id);  //return 1 if ok, 0 if not
void wa_stop_child(wa_child_t id);

#endif //PRESTO_WA_MANAGE_CHILDREN_H
