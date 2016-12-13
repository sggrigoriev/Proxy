//
// Created by gsg on 11/12/16.
//

#include <malloc.h>
#include <string.h>

#include "pr_ptr_list.h"

const unsigned int pr_get_get_ptr_list_len(char* const* list) {
    unsigned int len = 0;
    if(!list) return 0;
    while(list[len++]);
    return len;
}
char** pr_duplicate_ptr_list(char** dest, char* const * src) {
    size_t len,i;

    if(len = pr_get_get_ptr_list_len(src), !len) return NULL;
    dest = (char**)malloc(len*sizeof(char*));
    for(i = 0; i < len-1; i++) {
        dest[i] = strdup(src[i]);
    }
    dest[len-1] = NULL;
    return dest;
}
void pt_delete_ptr_list(char** ptr_list) {
    unsigned i = 0;
    if (!ptr_list) return;
    while (ptr_list[i]) free(ptr_list[i++]);
    free(ptr_list);     //delete NULL-terminator
}