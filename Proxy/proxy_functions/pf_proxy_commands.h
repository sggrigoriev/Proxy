//
// Created by gsg on 28/01/17.
//

#ifndef PRESTO_PF_PROXY_COMMANDS_H
#define PRESTO_PF_PROXY_COMMANDS_H

#include <stdlib.h>

#include "cJSON.h"
#include "pr_commands.h"

//Make answer from the message and put into buf. Returns buf addess
//TODO! Move to pr_commands
const char* pf_answer_to_command(cJSON* root, char* buf, size_t buf_size);


#endif //PRESTO_PF_PROXY_COMMANDS_H
