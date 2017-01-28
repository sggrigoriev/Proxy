//
// Created by gsg on 28/01/17.
//

#ifndef PRESTO_PF_PROXY_COMMANDS_H
#define PRESTO_PF_PROXY_COMMANDS_H

#include <stdlib.h>
#include "cJSON.h"

typedef struct pf_cmd_t {
    cJSON* obj;
    cJSON* cmd_array;
    char main_url[LIB_HTTP_MAX_URL_SIZE];
    int proxy_commands;
    int total_commands;
} pf_cmd_t;

//Return NULL if it is not a command
pf_cmd_t* pf_parse_cloud_commands(const char* cloud_msg);
void pf_close_cloud_commands(pf_cmd_t* cmd);

//Return 1 if there are some commands for the Proxy
int pf_are_proxy_commands(pf_cmd_t* cmd);
//Return 1 if there are some commands for the Agent
int pf_are_agent_commands(pf_cmd_t* cmd);

void pf_process_proxy_commands(pf_cmd_t* cmd);


#endif //PRESTO_PF_PROXY_COMMANDS_H
