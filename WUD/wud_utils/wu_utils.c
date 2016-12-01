//
// Created by gsg on 29/11/16.
//

#include <unistd.h>
#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <assert.h>
#include <malloc.h>

#include "wc_defaults.h"
#include "wu_utils.h"

static const char* create_file_name(char* buf, size_t buf_len, const char* dir, const char* fname, const char* ext);
static const unsigned int pl_len(char* const* list);

//return 0 if not exisst 1 if exists
int wu_process_exsists(const char* process_name) {
    char fn[4097];
    char pid[sizeof(pid_t)+1];
    FILE *f;
    int ret;

    f = fopen(create_file_name(fn, sizeof(fn)-1, WC_DEFAULT_PID_DIRECTORY, process_name, WC_DEFAULT_PIDF_EXTENCION), "r");
    if(!f) return 0;    //No pid file
    if(fgets(pid, sizeof(fn), f)) {
        snprintf(fn, sizeof(fn), "%s%s", WC_PROC_DIR, pid);
        ret = (access(fn, F_OK) != -1);
    }
    else {
        ret = 0; //No PID in file
    }
    fclose(f);
    return ret;
}

//return 0 if error, 1 if OK
int wu_create_pid_file(const char* process_name, pid_t process_pid) {
    char fn[4097];
    FILE* f;

    f = fopen(create_file_name(fn, sizeof(fn)-1, WC_DEFAULT_PID_DIRECTORY, process_name, WC_DEFAULT_PIDF_EXTENCION), "w+");
    if(!f) return 0;
     fprintf(f, "%d", process_pid);
    fclose(f);
    return 1;
}

//return 1 if empty, return 0 if not
int wu_dir_empty(const char* dir_name) {
    int n = 0;
    DIR *dir = opendir(dir_name);
    if (dir == NULL) //Not a directory or doesn't exist
        return 1;
    while (readdir(dir) != NULL) {
        if(++n > 2)
            break;
    }
    closedir(dir);
    if (n <= 2) //Directory Empty
        return 1;
    else
        return 0;
}

//return 1 if OK, 0 if error
int wu_clear_dir(const char* dir) {
    char path[WC_MAX_PATH];
    char* slash_mask;
    if(!strlen(dir)) return 1; //Nothing to elete

    slash_mask = (dir[strlen(dir)-1] == '/')?"*":"/*";

    snprintf(path, sizeof(path), dir, slash_mask);

    return (remove(path) == 0);
}

// return PID of OK 0 if not
pid_t wu_start_process(const char* prg_name, char* const* command_string, const char* process_working_dir) {
    pid_t pid = 0;
    assert(prg_name);

    if((pid = fork())) {
        return pid;     //Parent return
    }
    else {
        if(process_working_dir) chdir(process_working_dir);
        execv(prg_name, command_string);  //launcher exits and disapears...
    }
    return pid;
}
//Add program name as the first parameter and NULL as the last one
char** wu_prepare_papams_list(char** buf, const char* program_name, char* const* params_list) {
    unsigned int len=0;
    unsigned int j=0;
    assert(program_name);

    len = pl_len(params_list);

    buf = (char**)malloc((len+1)*sizeof(char*));
    buf[0]= strdup(program_name);
    for(j = 1; j < len-1; j++) buf[j] = strdup(params_list[j-1]);
    buf[len-1] = NULL;
    return buf;
}
char** wu_duplicate_params_list(char** dest, char* const* src) {
    size_t len,i;

    if(len = pl_len(src), !len) return NULL;
    dest = (char**)malloc(len*sizeof(char*));
    for(i = 0; i < len-1; i++) {
        dest[i] = strdup(src[i]);
    }
    dest[len-1] = NULL;
    return dest;
}
void wu_delete_params_list(char** params_list) {
    unsigned i = 0;
    if(!params_list) return;
    while(params_list[i]) free(params_list[i++]);
    free(params_list);
}
//////////////////////////////////////////////////////////////////////////
//Local finctions definition
static const char* create_file_name(char* buf, size_t buf_len, const char* dir, const char* fname, const char* ext) {
    snprintf(buf, buf_len-1, "%s%s.%s", dir, fname, ext);
    return buf;
}
static const unsigned int pl_len(char* const*  list) {
    unsigned int len = 0;
    if(!list) return 0;
    while(list[len++]);
    return len;
}