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
    Created by gsg on 30/11/16.

    Contains Proxy & Agent info and access functions
*/

#ifndef PRESTO_WM_CHILDS_INFO_H
#define PRESTO_WM_CHILDS_INFO_H

#include <sys/types.h>

#include "pr_commands.h"

/* Child process descriptor */
typedef struct {
    char* process_name;
    char* binary_name;
    char* working_directory;
    char** start_parameters;
    unsigned int watchdog_to;
    pid_t pid;
} wm_child_descriptor_t;

/***********************************************************************
 *  Create child process descriptor. Child descriptors just updates their PIDs during the start, so created in WUD
 *  startup they'd never been deleted!
 * @param pn        - process name
 * @param bn        - process executable file name
 * @param wd        - process working directory
 * @param sp        - process start parameters
 * @param wd_to     - process watchdog timeout in seconds
 * @param pid       - process PID
 * @return  - the descriptor index if OK, PR_CHILD_SIZE if error
 */
pr_child_t wm_create_cd(const char* pn, const char* bn, const char* wd, char* const* sp, unsigned int wd_to, pid_t pid);

/***********************************************************************
 * Get process' PID by descriptor's index
 * @param idx   - process' descriptor index
 * @return  - PID or 0 if somethig wrong
 */
pid_t wm_child_get_pid(pr_child_t idx);

/***********************************************************************
 * Set process PID in its descriptor
 * @param idx   - process' descriptor index
 * @param pid   - 0 process' PID
 * @return  - 1 if OK, 0 if not
 */
int wm_child_set_pid(pr_child_t idx, pid_t pid);

/***********************************************************************
 * Get process executable name from descriptor
 * @param idx   - process' descriptor index
 * @return  - return name or NULL if IDX invalid
 */
const char* wm_child_get_binary_name(pr_child_t idx);

/***********************************************************************
 * Get process' parameters
 * @param idx   - process' descriptor index
 * @return  - params list (could be empty) or NULL if IDX is invalid
 */
char** const wm_child_get_start_parameters(pr_child_t idx);

/***********************************************************************
 * Get process' working directory
 * @param idx   - process' descriptor index
 * @return  - working directory or NULL if invalid IDX
 */
const char* wm_child_get_working_directory(pr_child_t idx);

/***********************************************************************
 * Get process watchdog timeout
 * @param idx   - process' descriptor index
 * @return  - timeout in seconds or 0 if invalid IDX
 */
unsigned int wm_child_get_child_to(pr_child_t idx);

/***********************************************************************
 * Get process name
 * @param idx   - process' descriptor index
 * @return  - process name or NULL if unvalid index
 */
const char* wm_get_child_name(pr_child_t idx);
/*return idx or PR_CHILD_SIZE
***********************************************************************
 * Get descriptor index by process name
 * @param name  - process name
 * @return  - idx or PR_CHILD_SIZE if name was not found
 */
pr_child_t wm_get_child_descr(const char* name);

#endif /* PRESTO_WM_CHILDS_INFO_H */
