/*
// Created by gsg on 01/12/16.
*/
#include <stdio.h>

#include "pc_config.h"


const char* cfg_fname = "/media/sf_GWDrive/Presto_new/lib/pc_config/test.conf";

const char* string_item = "STRING_ITEM";
const char* string_empty_item = "STRING_EMPTY_ITEM";
const char* uint_item = "UINT_ITEM";
const char* string_array = "STRING_ARRAY";
const char* empty_array = "EMPTY_ARRAY";

int main() {
    char buf[1024];
    unsigned int uint;
    char** str_array;
    char** emp_array;

    cJSON* cfg = load_file(cfg_fname);
    if(!cfg) {
        printf("Cfg obj from file %s was not created. Stop.\n", cfg_fname);
        return 1;
    }
/* STRING_ITEM*/
    if(!getStrValue(cfg, string_item, buf, sizeof(buf)-1)) {
        printf("Error reading %s item. Stop.\n", string_item);
        return 1;
    }
    else {
        printf("%s = %s\n", string_item, buf);
    }
/* STRING_EMPTY_ITEM */
    if(!getStrValue(cfg, string_empty_item, buf, sizeof(buf)-1)) {
        printf("Error reading %s item. Stop.\n", string_empty_item);
        return 1;
    }
    else {
        printf("%s = %s\n", string_empty_item, buf);
    }
/* UINT_ITEM */
    if(!getUintValue(cfg, uint_item, &uint)) {
        printf("Error reading %s item. Stop.\n", uint_item);
        return 1;
    }
    else {
        printf("%s = %d\n", uint_item, uint);
    }
/* STRING_ARRAY */
    if(!getCharArray(cfg, string_array, &str_array, &uint)) {
        printf("Error reading %s item. Stop.\n", string_array);
        return 1;
    }
    else {
        unsigned i = 0;
        while(i < uint) {
            printf("%s[%d] = %s\n", string_array, i, str_array[i]);
            i++;
        }
    }
/* EMPTY_ARRAY */
    if(!getCharArray(cfg, empty_array, &emp_array, &uint)) {
        printf("Error reading %s item. Stop.\n", empty_array);
        return 1;
    }
    else {
        unsigned i = 0;
        while(i < uint) {
            printf("%s[%d] = %s\n", empty_array, i, emp_array[i]);
            i++;
        }
        printf("%s size = %d", empty_array, uint);
    }

    return 0;
}