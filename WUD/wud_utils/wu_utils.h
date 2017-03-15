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
    Created by gsg on 29/11/16.
    Some interfaces for system functions If Proxy and Agent strt use it - would be shifted to lib
*/

#ifndef PRESTO_WU_UTILS_H
#define PRESTO_WU_UTILS_H

#include <sys/types.h>

/*************************************************
 * Check the file <process_name>.<ext> in the directory specified
 * @param process_name      - process' name to chrck
 * @return  - 1 if exsists, 0 if not
 */
int wu_process_exsists(const char* process_name);

/*************************************************
 * Create the file with the process pid
 * @param process_name  - process name
 * @param process_pid   - process PID
 * @return  - 1 if OK, 0 if not
 */
int wu_create_pid_file(const char* process_name, pid_t process_pid);

/*************************************************
 * Check if the directory empty
 * @param dir_name  - checked directory name with path
 * @return  - 1 if empty, 0 if not
 */
int wu_dir_empty(const char* dir_name);

/*************************************************
 * Clean the directory contents (The favorite YC's function ;-)
 * @param dir_name  - directory name with path
 * @return  - 1 if OK, 0 if error
 */
int wu_clear_dir(const char* dir_name);

//Fork launcher; return PID. Launcher change working dir and run process with the command string
/*************************************************
 * Start chilg process bu fork. Launcher change working dir and run process with the command string
 * @param prg_name              - process to start
 * @param command_string        - process' start parameters
 * @param process_working_dir   - process' working directory
 * @return  - running process PID or 0 if error
 */
pid_t wu_start_process(const char* prg_name, char* const* command_string, const char* process_working_dir);

/*************************************************
 * Move all files found from src_folder to dst_folder
 * @param dest_folder   - destination folder
 * @param src_folder    - source folder
 * @return  - 1 if OK, 0 if not
 */
int wu_move_files(const char* dest_folder, const char* src_folder);

/*************************************************
 * Move file by copy & delete the source one. Rename after that (For firmware upgrade procedure)
 * @param old_dir       - source directory
 * @param old_name      - source file name
 * @param new_dir       - destination directory
 * @param new_name      - filal file name
 * @return  - 1 if OK, 0 id not
 */
int wu_move_n_rename(const char* old_dir, const char* old_name, const char* new_dir, const char* new_name);

/*************************************************
 * Extract the file name from the tail of string with path or URL (FOr firmware upgrade procedure)
 * @param path_or_url   - source string
 * @return  - poinetr to the first non-filename symbol from the tail or empty string
 */
const char* wu_cut_off_file_name(const char* path_or_url);

/*************************************************
 * Get the files list from directory
 * @param path  - path to the directory
 * @return  - NULL - terminated list
 */
char** wu_get_flist(const char* path);

/*************************************************
 * Free the NILL-terminated strings list
 * @param flist     - strings list
 */
void wu_free_flist(char** flist);

//Creates file name with full path.
/*************************************************
 * Construct the file name with the path from path, name & extention
 * @param buf       - buffer to store the result string
 * @param buf_size  - buffer size
 * @param dir       - path
 * @param fname     - file name
 * @param ext       - extention or empty string
 * @return  - pointer to the buffer
 */
const char* wu_create_file_name(char* buf, size_t buf_size, const char* dir, const char* fname, const char* ext);

#endif /* PRESTO_WU_UTILS_H */

