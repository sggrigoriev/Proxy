//
// Created by gsg on 29/11/16.
//

#ifndef PRESTO_WU_UTILS_H
#define PRESTO_WU_UTILS_H

int wu_process_exsists(const char* process_name); //return 0 if not exisst 1 if exists
int wu_create_pid_file(const char* process_name, pid_t process_pid); //return 0 if error, 1 if OK

int wu_dir_empty(const char* dir_name);  //return 1 if empty, return 0 if not
int wu_clear_dir(const char* dir_name);  //return 1 if OK, 0 if error

//Fork launcher; return PID. Launcher change working dir and run process with the command string
pid_t wu_start_process(const char* prg_name, char* const* command_string, const char* process_working_dir);

//Add program name as the first parameter and NULL as the last one
char** wu_prepare_papams_list(char** buf, const char* program_name, char* const* params_list);
char** wu_duplicate_params_list(char** dest, char* const * src);
void wu_delete_params_list(char** params_list);
#endif //PRESTO_WU_UTILS_H
