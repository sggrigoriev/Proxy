//
// Created by gsg on 11/12/16.
//
//Functions managing NULL-terminated pointers list

#ifndef PRESTO_PR_PTR_LIST_H
#define PRESTO_PR_PTR_LIST_H

//NB! the last NULL element also counted, i.e. empty list has len = 1
const unsigned int pr_get_get_ptr_list_len(char* const* list);
char** pr_duplicate_ptr_list(char** dest, char* const * src);
void pt_delete_ptr_list(char** ptr_list);

#endif //PRESTO_PR_PTR_LIST_H
