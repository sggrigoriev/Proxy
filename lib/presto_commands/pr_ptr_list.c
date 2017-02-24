//
// Created by gsg on 11/12/16.
//

#include <malloc.h>
#include <string.h>
#include <assert.h>

#include "pr_ptr_list.h"

const unsigned int pr_get_get_ptr_list_len(char* const* list) {
    unsigned int len = 0;

    assert(list);

    while(list[len]) len++;
    return len;
}
char** pr_duplicate_ptr_list(char** dest, char* const * src) {
    size_t len, i;

    assert(src);

    len = pr_get_get_ptr_list_len(src);
    dest = (char**)malloc((len+1)*sizeof(char*));
    for(i = 0; i < len; i++) {
        dest[i] = strdup(src[i]);
    }
    dest[len] = NULL;
    return dest;
}
char** pr_push(char** dest, const char* el, char* const* src) {
    assert(el);
    assert(src);

    unsigned int len = pr_get_get_ptr_list_len(src);
    dest = (char**)malloc((len+2)*sizeof(char*)); //pace for inserted element and termintator
    dest[0]= strdup(el);
    unsigned int j;
    for(j = 0; j < len; j++) dest[j+1] = strdup(src[j]);
    dest[len+1] = NULL;
    return dest;

}
void pt_delete_ptr_list(char** ptr_list) {
    unsigned i = 0;
    if (!ptr_list) return;
    while (ptr_list[i]) free(ptr_list[i++]);
    free(ptr_list);
}
const char* pr_ptr_list2string(char* buf, size_t size, char* const* list) {
    assert(buf);
    assert(size);
    assert(list);

    strncpy(buf, "{", size-1);
    unsigned int i;
    for(i = 0; i < pr_get_get_ptr_list_len(list); i++) {
        strncat(buf, "<", size-strlen(buf)-1);
        strncat(buf, list[i], size-strlen(buf)-1);
        strncat(buf, ">", size-strlen(buf)-1);
    }
    strncat(buf, "}", size-strlen(buf)-1);
    buf[size-1] = '\0';
    return buf;
}
