//
// Created by gsg on 29/11/16.
//

#ifndef PRESTO_WU_UTILS_H
#define PRESTO_WU_UTILS_H

#include <sys/types.h>

int wu_process_exsists(const char* process_name); //return 0 if not exisst 1 if exists
int wu_create_pid_file(const char* process_name, pid_t process_pid); //return 0 if error, 1 if OK

int wu_dir_empty(const char* dir_name);  //return 1 if empty, return 0 if not
int wu_clear_dir(const char* dir_name);  //return 1 if OK, 0 if error

//Fork launcher; return PID. Launcher change working dir and run process with the command string
pid_t wu_start_process(const char* prg_name, char* const* command_string, const char* process_working_dir);

//Move all files found from src_folder to dst_folder
//Returns 1 if OK, 0 if not
int wu_move_files(const char* dest_folder, const char* src_folder); //Returns 1 if OK, 0 if not

//Returns 1 if Ok. All diagnostics inside
int wu_move_n_rename(const char* old_dir, const char* old_name, const char* new_dir, const char* new_name);
//Return poinetr to the first non-filename symbol from the tail or empty string. No NULL!
const char* wu_cut_off_file_name(const char* path_or_url);
//
//return files list from the directory. If no files - return NULL
char** wu_get_flist(const char* path);
void wu_free_flist(char** flist);
//Creates file name with full path.
const char* wu_create_file_name(char* buf, size_t buf_size, const char* dir, const char* fname, const char* ext);

#endif //PRESTO_WU_UTILS_H

