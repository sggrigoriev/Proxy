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

 Wrappper under JSON library for basic operations with configuration file
*/

#ifndef PRESTO_PC_CONFIG_H
#define PRESTO_PC_CONFIG_H

#include "cJSON.h"

/**
 * Load config file "fname" and parse it.
 *
 * @param fname - configuration file name (most likely - in JSON format)
 * @return  - pointer to cJSON object or NULL if error.
 *          NB! if config file is empty - return the empty cJSON object - "{}" parsing result
*/
cJSON* load_file(const char* fname);

/**
 * Insert or update configuration string item
 *
 * @param item_name - JSON item's name
 * @param value     - new value for the item
 * @param cfg       - parsed configuration
*/
void json_str_update(const char* item_name, const char* value, cJSON* cfg);

/**
 * Insert or update configuration uint item
 *
 * @param item_name - JSON item's name
 * @param value     - new value for the item
 * @param cfg       - parsed configuration
*/
void json_uint_update(const char* item_name, unsigned int value, cJSON* cfg);

/**
 * Get value of string item
 *
 * @param cfg           - parsed configuration
 * @param field_name    - item name
 * @param str_setting   - buffer for returned value
 * @param max_size      - max buffer size
 * @return  - 1 if OK 0 if error (not found, wrong type, small buffer)
*/
int getStrValue(cJSON* cfg, const char* field_name, char* str_setting, size_t max_size);

/**
 * Get value of uint item
 *
 * @param cfg           - parsed configuration
 * @param field_name    - item name
 * @param uint_setting  - buffer for returned value
 * @return  - 1 if OK 0 if error (not found, wrong type)
*/
int getUintValue(cJSON* cfg, const char* field_name, unsigned int* uint_setting);

/**
 * Get value of array of strings type. NB! Memory allocates inside!
 *
 * @param cfg           - parsed configuration
 * @param field_name    - item name
 * @param carr_setting  - array of strings returned
 * @return  - 1 if OK 0 if error
*/
int getCharArray(cJSON* cfg, const char* field_name, char*** carr_setting);

/**
 * Save the cfg object, translated to JSON string into text file fname.
 *
 * @param fname - file the config will be saved to
 * @param cfg   - parsed configuration
 * @return  - 1 if success and 0 if error
*/
int saveToFile(const char* fname, cJSON* cfg);

/**
 * Update the correspondent setting old_value in memory plus updates the settings file: insert/update field name value by new_value
 *
 * @param func_name     - calling function name foe logging
 * @param conf_fname    - configuration file name
 * @param field_name     - filend name
 * @param new_value      - new value
 * @param old_value      - poiner to the setting in memory
 * @param max_size       - old_value capacity
 * @return  - 1 if Ok, 0 if error
*/
int saveStrValue(const char* func_name, const char* conf_fname, const char* field_name, const char *new_value, char* old_value, size_t max_size);

/**
 * Update the correspondent setting old_value in memory plus updates the settings file: insert/update field name value by new_value
 *
 * @param func_name     - calling function name foe logging
 * @param conf_fname    - configuration file name
 * @param field_name    - filend name
 * @param new_value     - new value
 * @param old_value     - poiner to the setting in memory
 * @return  - 1 of Ok, 0 if error
*/
int saveUintValue(const char* func_name, const char* conf_fname, const char* field_name,  unsigned int new_value, unsigned int* old_value);

/************************************************************************************/
/**
 * Read file contains one string w/o linefeeds
 *
 * @param file_name     - one string file name
 * @param value         - buffer
 * @param size          - buffer size
 * @param value_name    - variable name. Just for logging error messages
 * @return  - 1 id Ok, 0 if error
 */
int read_one_string_file(const char* file_name, char* value, size_t size, const char* value_name);
/**
 * Save new value of value name paramater into file_name. NB! No linefeeds!
 *
 * @param file_name     - one string file name
 * @param new_val       - string value to be saved
 * @param value_name    - parameter name
 * @return  - 1 of Ok, 0 if not
 */
int save_one_string_file(const char* file_name, const char* new_val, const char* value_name);
/**
 * Print build version info based on cMake global variables
 *
 * @param version   - the version number from the corresponding include
 * @param buf       - buffer or output
 * @param size      - buffer size
 * @return  - pointer to the buffer:all this stuff about version, branch and so on
 */
const char* get_version_printout(const char* version, char* buf, size_t size);

#endif /*PRESTO_PC_CONFIG_H */
