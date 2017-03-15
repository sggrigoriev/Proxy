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

 Wrappper under JSON library for basic operations with comfiguration file
*/

#ifndef PRESTO_PC_CONFIG_H
#define PRESTO_PC_CONFIG_H

#include "cJSON.h"

/*Load config file "fname" and parse it.
  fname   - configuration file name (most likely - in JSON format)
Return pointer to cJSON object or NULL if error.
NB! if config file is empty - return the empty cJSON object - "{}" parsing result
*/
cJSON* load_file(const char* fname);

/*Insert or update configuration string item
  item_name   - JSON item's name
  value       - new value for the item
  cfg         - parsed configuration
*/
void json_str_update(const char* item_name, const char* value, cJSON* cfg);

/*Insert or update configuration uint item
  item_name   - JSON item's name
  value       - new value for the item
  cfg         - parsed configuration
*/
void json_uint_update(const char* item_name, unsigned int value, cJSON* cfg);

/*Get value of string item
  cfg         - parsed configuration
  field_name  - item name
  str_setting - buffer for returned value
  max_size    - max buffer size
Return 1 if OK 0 if error (not found, wrong type, small buffer)
*/
int getStrValue(cJSON* cfg, const char* field_name, char* str_setting, size_t max_size);

/*Get value of uint item
  cfg         - parsed configuration
  field_name  - item name
  uint_setting - buffer for returned value
Return 1 if OK 0 if error (not found, wrong type)
*/
int getUintValue(cJSON* cfg, const char* field_name, unsigned int* uint_setting);

/*Get value of array of strings type. NB! Memory allocates inside!
  cfg             - parsed configuration
  field_name      - item name
  carr_setting    - array of strings returned
Return 1 if OK 0 if error
*/
int getCharArray(cJSON* cfg, const char* field_name, char*** carr_setting);

/*Safe the cfg object, translated to JSON string into text file fname.
  fname   - file the config will be saved to
  cfg     - parsed configuration
 Return 1 if success and 0 if error
*/
int saveToFile(const char* fname, cJSON* cfg);

/*Update the correspondent setting old_value in memory plus updates the settings file: insert/update field name value by new_value
  func_name       - calling function name foe logging
  conf_fname      - configuration file name
  field_name      - filend name
  new_value       - new value
  old_value       - poiner to the setting in memory
  max_size        - old_value capacity
Return 1 if Ok, -1 if error
*/
int saveStrValue(const char* func_name, const char* conf_fname, const char* field_name, const char *new_value, char* old_value, size_t max_size);

/*Update the correspondent setting old_value in memory plus updates the settings file: insert/update field name value by new_value
func_name     - calling function name foe logging
conf_fname    - configuration file name
field_name    - filend name
new_value     - new value
old_value     - poiner to the setting in memory
*/
int saveUintValue(const char* func_name, const char* conf_fname, const char* field_name,  unsigned int new_value, unsigned int* old_value);


#endif /*PRESTO_PC_CONFIG_H */
