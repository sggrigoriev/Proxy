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
 */
/*
    Created by gsg on 11/12/16.

    Functions managing NULL-terminated pointers list. Used to operate with started child processes' parameters
*/

#ifndef PRESTO_PR_PTR_LIST_H
#define PRESTO_PR_PTR_LIST_H

/* Get the size of the list.
 *      list    - pointer to the list. NB! can not be NULL!
 *  Return amout of alements, i.e. size-1
*/
const unsigned int pr_get_get_ptr_list_len(char* const* list);

/* Make a copy of source list
 *      dest    - pointer to new new list
 *      src     - source list
 *  Return the dest pointer
*/
char** pr_duplicate_ptr_list(char** dest, char* const * src);

/* Add the first element to the list. NB! both - src & dest lists are valid after the operation!
 *      dest    - pointer to the result
 *      el      - element added
 *      src     - source list
 *  Return the pointer to the dest
*/
char** pr_push(char** dest, const char* el, char* const* src);

/* Delete the list. NB! NULL as a parameter allowed
 *  ptr_list    - pointer to the list to be deleted *
*/
void pt_delete_ptr_list(char** ptr_list);

/* Encode the list into the string: {<el0><el1>...<elN>}
 *      buf     - buffer for result string
 *      size    - buffer size
 *      list    - pointer to the list
 *  Return pointer to the buf
*/
const char* pr_ptr_list2string(char* buf, size_t size, char* const* list);

#endif /*PRESTO_PR_PTR_LIST_H*/
