//
// Created by gsg on 11/12/16.
//
//Functions managing NULL-terminated pointers list

#ifndef PRESTO_PR_PTR_LIST_H
#define PRESTO_PR_PTR_LIST_H

//Return len, i.e. size-1
const unsigned int pr_get_get_ptr_list_len(char* const* list);
char** pr_duplicate_ptr_list(char** dest, char* const * src);
char** pr_push(char** dest, const char* el, char* const* src);  //Add the first element
void pt_delete_ptr_list(char** ptr_list);

const char* pr_ptr_list2string(char* buf, size_t size, char* const* list); //{<el0><el1>...<elN>}

#endif //PRESTO_PR_PTR_LIST_H
