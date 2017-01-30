//
// Created by gsg on 28/01/17.
//

#ifndef PRESTO_PF_PROXY_COMMANDS_H
#define PRESTO_PF_PROXY_COMMANDS_H

#include <stdlib.h>
#include "cJSON.h"

typedef struct pf_cmd_t {
    cJSON* obj;
    cJSON* header;
    cJSON* agent_cmd_array;
    cJSON* proxy_cmd_array;
} pf_cmd_t;

//Return NULL if it is not a command
pf_cmd_t* pf_parse_cloud_commands(const char* cloud_msg);
void pf_close_cloud_commands(pf_cmd_t* cmd);

void pf_process_proxy_commands(pf_cmd_t* cmd);

void pf_encode_agent_commands(pf_cmd_t* cmd, char* resp, size_t size);

//Make answer from the message and put into buf. Returns buf addess
const char* pf_answer_to_command(cJSON* root, char* buf, size_t buf_size);


#endif //PRESTO_PF_PROXY_COMMANDS_H
