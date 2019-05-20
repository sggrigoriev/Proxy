/*
 *  Copyright 2018 People Power Company
 *
 *  This code was developed with funding from People Power Company
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
*/
/*
 Created by gsg on 19/05/19.
*/
#include <string.h>

#include "cJSON.h"
#include "pu_logger.h"
#include "pc_settings.h"

#include "pc_defaults.h"
#include "cc_emulator.h"

typedef enum {CC_UNDEF = 0, CC_PERMIT1 = 1, CC_PERMIT2 = 2, CC_MAX_SIZE} cc_command_type_t;

const char* cmd = "commands";
const char* cmd_id = "commandId";
const char* par = "parameters";
const char* nam = "name";
const char* val = "value";
const char* perm = "permitJoining";

static const char* get_command_id(cJSON* msg, char* command_id, size_t max_len) {
    command_id[0] = '\0';

    cJSON* cmd_arr = cJSON_GetObjectItem(msg, cmd);
    if(!cmd_arr) goto on_exit;
    cJSON* cmd_elem = cJSON_GetArrayItem(cmd_arr, 0);
    if(!cmd_elem) goto on_exit;
    cJSON* cmd_id_obj = cJSON_GetObjectItem(cmd_elem, cmd_id);
    if(!cmd_id_obj) goto on_exit;
    snprintf(command_id, max_len-1, "%d", cmd_id_obj->valueint);
on_exit:
    return command_id;
}

static const char* make_permit_ans(cc_command_type_t permitNo, const char* command_id, char* out_msg, size_t max_len) {
    const char* ans = "{\"responses\":[{\"commandId\":%s,\"result\":1}],\"proxyId\":\"%s\",\"sequenceNumber\":100050}";
    char proxy_id[PROXY_DEVICE_ID_SIZE+1];
    pc_getProxyDeviceID(proxy_id, sizeof(proxy_id));

    snprintf(out_msg, max_len-1, ans, command_id, proxy_id);
    return out_msg;
}

static cc_command_type_t wtf(cJSON* in) {
    cJSON* cmd_arr = cJSON_GetObjectItem(in, cmd);
    if(!cmd_arr) return CC_UNDEF;

    cJSON* cmd_elem = cJSON_GetArrayItem(cmd_arr, 0);
    if(!cmd_elem) return CC_UNDEF;

    cJSON* par_arr = cJSON_GetObjectItem(cmd_elem, par);
    if(!par_arr) return CC_UNDEF;

    cJSON* permit = cJSON_GetArrayItem(par_arr, 0);
    if(!permit) return CC_UNDEF;

    cJSON* p_nam = cJSON_GetObjectItem(permit, nam);
    cJSON* p_val = cJSON_GetObjectItem(permit, val);
    if(!p_nam || !p_val) return CC_UNDEF;
    if(strcmp(p_nam->valuestring, perm)!= 0) return CC_UNDEF;
    if(!strcmp(p_val->valuestring, "1")) return CC_PERMIT1;
    if(!strcmp(p_val->valuestring, "2")) return CC_PERMIT2;
    return CC_UNDEF;
}

int is_eateable(const char* in_msg) {
    int ret = 0;
    cJSON* obj = cJSON_Parse(in_msg);
    if(obj) ret = (wtf(obj) != CC_UNDEF);
    cJSON_Delete(obj);
    return ret;
}

const char* make_answer(const char* in_msg, char* out_msg, size_t max_len) {
    cJSON* msg = cJSON_Parse(in_msg);
    if(!msg) {
        pu_log(LL_ERROR, "%s: Error JSON parsing for %s.", __FUNCTION__, in_msg);
        return NULL;
    }
    cc_command_type_t ct = wtf(msg);
    switch (ct) {
        case CC_PERMIT1:
        case CC_PERMIT2: {
            char command_id[100];
            get_command_id(msg, command_id, sizeof(command_id));
            make_permit_ans(ct, command_id, out_msg, max_len);
        }
            break;
        case CC_UNDEF:
        default:
            pu_log(LL_WARNING, "%s: Can't recognise syntax of %s", __FUNCTION__, in_msg);
            out_msg[0] = '\0';
            break;
    }
    cJSON_Delete(msg);
    return out_msg;
}

const char* get_mesure(char* out_buf, size_t max_len) {
    const char* ans = "{\"measures\":[{\"deviceId\":\"%s\",\"paramsMap\":{\"permitJoining\":\"1\"}}],\"proxyId\":\"%s\",\"sequenceNumber\":100051}";
    char proxy_id[PROXY_DEVICE_ID_SIZE+1];
    pc_getProxyDeviceID(proxy_id, sizeof(proxy_id));

    snprintf(out_buf, max_len-1, ans, proxy_id, proxy_id);
    return out_buf;
}