//
// Created by gsg on 28/11/16.
//

#ifndef PRESTO_PC_CONFIG_H
#define PRESTO_PC_CONFIG_H

#include "cJSON.h"

//Load config file "fname, parse it. Return pointer to cJSON object or NULL if error.
//NB! if config file is empty - return the empty cJSON object - "{}" parsing result
cJSON* load_file(const char* fname);

//Update string item_name by value in cfg cJSON object
//If the item doesn't exist add the item to the object
void json_str_update(const char* item_name, const char* value, cJSON* cfg);

//Update unsigned int item_name by value in cfg cJSON object
//If the item doesn't exist add the item to the object
void json_uint_update(const char* item_name, unsigned int value, cJSON* cfg);

//Copy string value of field_name of cgf object into str_settings limited by max_size.
//Return 1 if OK 0 if error
int getStrValue(cJSON* cfg, const char* field_name, char* str_setting, size_t max_size);

//Copy unsigned int value of field_name of cgf object into uint_setting.
//Return 1 if OK 0 if error
int getUintValue(cJSON* cfg, const char* field_name, unsigned int* uint_setting);

//Allocate memory and copy into it string array. arr_len contains amount of strings in array
//Return 1 if OK 0 if error
int getCharArray(cJSON* cfg, const char* field_name, char*** carr_setting, unsigned int* arr_len);

//Safe the cfg object, translated to json ASCII text into text file fname. Return 1 if success and 0 othervize
int saveToFile(const char* fname, cJSON* cfg);

//Update the correspondent setting old_value in memory plus updates the settings file: insert/update field name value by new_value
int saveStrValue(const char* func_name, const char* conf_fname, const char* field_name, const char *new_value, char* old_value, size_t max_size);

int saveUintValue(const char* func_name, const char* conf_fname, const char* field_name,  unsigned int new_value, unsigned int* old_value);


#endif //PRESTO_PC_CONFIG_H
