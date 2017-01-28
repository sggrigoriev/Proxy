//
// Created by gsg on 15/01/17.
//

#ifndef PRESTO_WA_ALARM_H
#define PRESTO_WA_ALARM_H

#include "pr_commands.h"

void wa_alarms_init();
void wa_alarm_reset(pr_child_t proc);
void wa_alarm_update(pr_child_t proc);

//Return 1 if timeout
int wa_alarm_wakeup(pr_child_t proc);

#endif //PRESTO_WA_ALARM_H
