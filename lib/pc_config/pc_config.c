/*
 *  Copyright 2017 People Power Company
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
 *

 Created by gsg on 28/11/16.

*/

#include <errno.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include "pc_config.h"
/**
 * Cut off the string until the first non-printable
 * @param str   - changed string
 * @return      - pointer to the str
 */
static char* cut_non_printables(char* s) {
    char* b = s;
    while(isprint(*b)) b++;
    *b = '\0';
    return s;
}

/*load_file()    - load configuration file into memory
fname           - file name
Return NULL if no file or bad JSON, or cJSON object
*/
cJSON* load_file(const char* fname) {
    FILE *f;
    char buffer[100];
    char* cfg = NULL;
    cJSON* ret = NULL;

    if(f = fopen(fname, "r"), f == NULL) {
        fprintf(stderr, "Config file %s open error %d %s\n", fname, errno, strerror(errno));
        return 0;
    }
    size_t ptr = 0;
    while (fgets(buffer, sizeof(buffer), f)) {
        cfg = realloc(cfg, strlen(buffer)+ptr);
        memcpy(cfg+ptr, buffer, strlen(buffer));
        ptr += strlen(buffer);
    }
    cfg = realloc(cfg, ptr+1);
    cfg[ptr] = '\0';
    fclose(f);

    if(!strlen(cfg)) {
        fprintf(stderr, "Configuration file %s is empty\n", fname);
        return cJSON_Parse("{}");
    }

    ret = cJSON_Parse(cfg);
    if(ret == NULL) {
        fprintf(stderr, "Error parsing the following:%s\n Error starts: %s", cfg, cJSON_GetErrorPtr());
    }
    free(cfg);
    return ret;
}

/*json_str_update() - update field of string type
item_name     - fileld name
value         - new value
cfg           - pointer to cJSON object
*/
void json_str_update(const char* item_name, const char* value, cJSON* cfg) {
    cJSON* at;

    if(at = cJSON_GetObjectItem(cfg, item_name), at == NULL) {         /*Add item */
        fprintf(stderr, "%s item is not found in configuretion file. Added.\n", item_name);
        cJSON_AddItemToObject(cfg, item_name, cJSON_CreateString(value));
    }
    else {
        free(at->valuestring);                                                      /*UPdate Item value */
        at->valuestring = strdup(value);
    }
}

/*json_uint_update() - update field of uint type
item_name     - fileld name
value         - new value
cfg           - pointer to cJSON object
*/
void json_uint_update(const char* item_name, unsigned int value, cJSON* cfg) {
    cJSON* at;

    if(at = cJSON_GetObjectItem(cfg, item_name), at == NULL) {         /*Add item */
        fprintf(stderr, "%s item is not found in configuretion file. Added.\n", item_name);
        cJSON_AddItemToObject(cfg, item_name, cJSON_CreateNumber(value));
    }
    else {
        at->valueint = value;        /*UPdate Item value */
    }
}

/*getStrValue() - get string value
cfg           - poiner to cJSON object containgng configuration
field_name    - JSON fileld name
str_setting   - returned value of field_name
max_size      - str_setting capacity
Return 1 if OK 0 if error
*/
int getStrValue(cJSON* cfg, const char* field_name, char* str_setting, size_t max_size) {
    cJSON* obj;
    if(obj = cJSON_GetObjectItem(cfg, field_name), obj == NULL) {
        fprintf(stderr, "Setting %s is not found ", field_name);
        return 0;
    }
    if(obj->type!= cJSON_String) {
        fprintf(stderr, "Setting %s is not a string.\n", field_name);
        return 0;
    }
    if(strlen(obj->valuestring) > max_size-1) {
        fprintf(stderr, "Setting %s value > than max size: %zu against %zu.\n", field_name, strlen(field_name), max_size);
        return 0;
    }
    strcpy(str_setting, obj->valuestring);
    return 1;
}

/*getUintValue()    - get uint value
cfg           - poiner to cJSON object containgng configuration
field_name    - JSON fileld name
uint_setting   - returned value of field_name
Return 1 if OK 0 if error
*/
int getUintValue(cJSON* cfg, const char* field_name, unsigned int* uint_setting) {
    cJSON *obj;
    if (obj = cJSON_GetObjectItem(cfg, field_name), obj == NULL) {
        fprintf(stderr, "Setting %s is not found ", field_name);
        return 0;
    }
    if(obj->type!= cJSON_Number) {
        fprintf(stderr, "Setting %s is not a numeric.\n", field_name);
        return 0;
    }
    *uint_setting = (unsigned int)obj->valueint;
    return 1;
}

/*getCharArray()    -Allocate memory and copy into it string array. arr_len contains amount of strings in array
cfg           - poiner to cJSON object containgng configuration
field_name    - JSON fileld name
carr_setting  - NULL-terminated list of char*
Return 1 if OK 0 if error
*/
int getCharArray(cJSON* cfg, const char* field_name, char*** carr_setting) {
    cJSON *obj;
    unsigned int i,list_len;

    *carr_setting = NULL;

    if (obj = cJSON_GetObjectItem(cfg, field_name), obj == NULL) {
        fprintf(stderr, "Setting %s is not found ", field_name);
        return 0;
    }
    if(obj->type != cJSON_Array) {
        fprintf(stderr, "Setting %s is not an array.\n", field_name);
        return 0;
    }
    list_len = (unsigned int)cJSON_GetArraySize(obj);

     /* One position for NULL-terminator */
    (*carr_setting) = (char**)malloc((list_len+1)*sizeof(char**)); /* One position added for NULL-terminator */
    for(i = 0; i < list_len; i++) {
        cJSON* item;
        item = cJSON_GetArrayItem(obj, i);
        if(item->type != cJSON_String) {
            fprintf(stderr, "Item %d of setting %s not a string.Parameter ignored.\n", i, field_name);
            free(*carr_setting);
            *carr_setting = NULL;
            return 0;
        }
        (*carr_setting)[i] = (!item->valuestring)?strdup(""):strdup(item->valuestring);
    }
    (*carr_setting)[i] = NULL;  /* add termination element into the list */
    return 1;
}

/*saveToFile() - save cfg to the fname
fname     - file name
cfg       - pointer to cJSON object
*/
int saveToFile(const char* fname, cJSON* cfg) { /*Returns 0 if bad */
    FILE *f;
    int ret = 0;
    char temp_name[L_tmpnam];

    strcpy(temp_name, "CONFIG_TEMPXXXXXX");

    close(mkstemp(temp_name));
    if(f = fopen(temp_name, "w"), f == NULL) {
        fprintf(stderr, "Saving config file: Unable to open temp file. Save Failed %d %s\n", errno, strerror(errno));
    }
    else {
        char* prnt_str = cJSON_Print(cfg);
        fprintf(f,"%s\n", cJSON_Print(cfg));
        fclose(f);
        sync();
        free(prnt_str);
        if(unlink(fname)) {
            fprintf(stderr, "Can\'t delete the file %s. Changed config file saved in %s. %d %s\n", fname, temp_name, errno, strerror(errno));
        }
        else if(rename(temp_name, fname)) {
            fprintf(stderr, "Can\'t rename the file %s to %s. Changed config file saved in %s. %d %s\n", temp_name, fname, temp_name, errno, strerror(errno));
        }
        else
            ret = 1;
    }
    cJSON_Delete(cfg);
    return ret;
}

/*saveStrValue() - save string type value to the fname
func_name     - calling function name foe logging
conf_fname    - configuration file name
field_name    - filend name
new_value     - new value
old_value     - poiner to the setting in memory
max_size      - old_value capacity
*/
int saveStrValue(const char* func_name, const char* conf_fname, const char* field_name, const char *new_value, char* old_value, size_t max_size) {
    if(strlen(new_value)+1 > max_size) {
	fprintf(stderr, "%s(): new value %s is too big: %zu against %zu\n", func_name, new_value, strlen(new_value), max_size);
        return 0;
    }

    strcpy(old_value, new_value);     /* Save into loaded settings */

    cJSON* cfg = load_file(conf_fname);
    if(cfg == NULL) return 0;

    json_str_update(field_name, new_value, cfg);
    return saveToFile(conf_fname, cfg);
}

/*saveUintValue() - save uint type value to the fname
func_name     - calling function name foe logging
conf_fname    - configuration file name
field_name    - filend name
new_value     - new value
old_value     - poiner to the setting in memory
*/
int saveUintValue(const char* func_name, const char* conf_fname, const char* field_name,  unsigned int new_value, unsigned int* old_value) {
    *old_value = new_value;     /* Save into loaded settings */

    cJSON* cfg = load_file(conf_fname);
    if(cfg == NULL) return 0;

    json_uint_update(field_name, new_value, cfg);
    return saveToFile(conf_fname, cfg);
}

/************************************************************************************/
int read_one_string_file(const char* file_name, char* value, size_t size, const char* value_name) {
    FILE* f = fopen(file_name, "r");
    if(!f) {
        fprintf(stderr, "File %s open error %d, %s. %s got default value\n", file_name, errno, strerror(errno), value_name);
        goto on_err;
    }
    char* ptr = fgets(value, size, f);
    if(!ptr) {
        fprintf(stderr, "File %s read error %d, %s. %s got default value\n", file_name, errno, strerror(errno), value_name);
        goto on_err;
    }
    cut_non_printables(value);
    fclose(f);
    return 1;
on_err:
    value[0] = '\0';
    if(f) fclose(f);
    return 0;

}
int save_one_string_file(const char* file_name, const char* new_val, const char* value_name) {
    FILE* f = fopen(file_name, "w");
    if(!f) {
        fprintf(stderr, "File %s open error %d, %s. %s can't be pesistently saved!\n", file_name, errno, strerror(errno), value_name);
        return 0;
    }
    fprintf(f, "%s", new_val);
    fclose(f);
    sync();
    return 1;
}

/*
 * Return all this about version, branch and so on
 *
 */
const char* get_version_printout(const char* version, char* buf, size_t size) {
    snprintf(buf, size,
             "Built on %s at %s\n"
             "Git repository version %s\n"
             "Git commit: %s\n"
             "Git branch: %s\n"
             "\tUncommited: %s\n"
             "*** To repeat this build use:\n"
             "\tgit clone --single-branch -b  %s %s .\n"
             "\tgit fetch origin %s\n"
             "\tgit reset --hard FETCH_HEAD\n"
             ,
             __DATE__, __TIME__
             ,version
             ,GIT_COMMIT
             ,GIT_BRANCH
             ,(UNCOMMITED_CHANGES == 0) ? "NO": " !!!!! YES !!!!!!"
             ,GIT_BRANCH, GIT_URL
             ,GIT_COMMIT
            );
    return buf;
}