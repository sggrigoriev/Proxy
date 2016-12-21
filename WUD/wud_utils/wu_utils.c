//
// Created by gsg on 29/11/16.
//

#include <unistd.h>
#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <assert.h>
#include <malloc.h>
#include <stdlib.h>


#include "wc_defaults.h"
#include "pr_ptr_list.h"
#include "wu_utils.h"

static const char* create_file_name(char* buf, size_t buf_len, const char* dir, const char* fname, const char* ext);
static const char* add_slash(char* buf, size_t buf_size,const char* path); //Add slash to the path's tail if no slash found there

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

    if(!dir || !strlen(dir)) return 1; //Nothing to elete

    add_slash(path, sizeof(path), dir);

    snprintf(path+strlen(path), sizeof(path)-strlen(path), "%s%s%s", "rm -rf ", dir, "*");

    return (system(path) == 0);
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

//Move all files found from src_folder to dst_folder
//Returns 1 if OK, 0 if not
int wu_move_files(const char* dest_folder, const char* src_folder) { //Returns 1 if OK, 0 if not
    DIR           *d;
    struct dirent *dir;
    int ret = 1;

    if(d = opendir(src_folder), d) {
        char buf_src[PATH_MAX];
        char buf_dst[PATH_MAX];

        add_slash(buf_src, sizeof(buf_src), src_folder);
        size_t offs_src = strlen(buf_src);

        add_slash(buf_dst, sizeof(buf_dst), dest_folder);
        size_t offs_dst = strlen(buf_dst);

        while (dir = readdir(d), dir != NULL) {
            strncpy(buf_src + offs_src, dir->d_name, sizeof(buf_src) - offs_src);
            strncpy(buf_dst + offs_dst, dir->d_name, sizeof(buf_dst) - offs_dst);

            if(rename(buf_src, buf_dst)) {
                ret = 0;
                break;
            }
        }
        closedir(d);
    }
    else {
        ret = 0;
    }
    return ret;
}

//return files list from directory. Last element NULL. If no files - return at least one element with NULL
//return NULL if allocation error
char** wu_get_flist(const char* path) {
    DIR           *d;
    struct dirent *dir;
    char** ret = NULL;

    if(ret = malloc(sizeof(char**)), !ret) return NULL;
    unsigned int len = 0;
    if(d = opendir(path), d) {
        while (dir = readdir(d), dir != NULL) {
            if(ret[len] = strdup(dir->d_name), !ret[len]) {
                pt_delete_ptr_list(ret);
                return NULL;
            }
            len++;
            if(ret = realloc(ret, sizeof(char**)*len), !ret) {
                pt_delete_ptr_list(ret);
                return NULL;
            }

        }
        closedir(d);
        ret[len] = NULL;
    }
    else {
        ret[len] = NULL;
    }
    return ret;
}
void wu_free_flist(char** flist) {
    pt_delete_ptr_list(flist);
}
//////////////////////////////////////////////////////////////////////////
//Local finctions definition
static const char* create_file_name(char* buf, size_t buf_len, const char* dir, const char* fname, const char* ext) {
    if(strlen(ext))
        snprintf(buf, buf_len-1, "%s%s.%s", dir, fname, ext);
    else
        snprintf(buf, buf_len-1, "%s%s", dir, fname);
    return buf;
}
//Add slash to the path's tail if no slash found there
static const char* add_slash(char* buf, size_t buf_size, const char* path) {
    if(path[strlen(path)-1] == '/') return strncpy(buf, path, buf_size);
    snprintf(buf, buf_size, "%s%s", path, "/");
    return buf;
}
