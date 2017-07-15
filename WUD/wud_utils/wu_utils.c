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
*/

#include <unistd.h>
#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <assert.h>
#include <malloc.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>

#include "pu_logger.h"
#include "pr_ptr_list.h"

#include "wc_defaults.h"
#include "wu_utils.h"

/***********************************************************
 * Local function definition
 *
 * Add slash to the path's tail if no slash found there
 * @param buf       - buffer to store the result
 * @param buf_size  - buffer size
 * @param path      - the path to be changed
 * @return  - pointer to the buffer
 */
static const char* add_slash(char* buf, size_t buf_size,const char* path);

/***********************************************************
 * Public functions definition
 */

int wu_process_exsists(const char* process_name) {
    char fn[WC_MAX_PATH];
    char pid[sizeof(pid_t)+1];
    FILE *f;
    int ret;

    f = fopen(wu_create_file_name(fn, sizeof(fn)-1, WC_DEFAULT_PID_DIRECTORY, process_name, WC_DEFAULT_PIDF_EXTENCION), "r");
    if(!f) return 0;    /* No pid file */
    if(fgets(pid, sizeof(fn), f)) {
        snprintf(fn, sizeof(fn), "%s%s", WC_PROC_DIR, pid);
        ret = (access(fn, F_OK) != -1);
    }
    else {
        ret = 0; /* No PID in file */
    }
    fclose(f);
    return ret;
}

int wu_create_pid_file(const char* process_name, pid_t process_pid) {
    char fn[WC_MAX_PATH];
    FILE* f;

    f = fopen(wu_create_file_name(fn, sizeof(fn)-1, WC_DEFAULT_PID_DIRECTORY, process_name, WC_DEFAULT_PIDF_EXTENCION), "w+");
    if(!f) return 0;
     fprintf(f, "%d", process_pid);
    fclose(f);
    return 1;
}

int wu_dir_empty(const char* dir_name) {
    int n = 0;
    DIR *dir = opendir(dir_name);
    if (dir == NULL)        /* Not a directory or doesn't exist */
        return 1;
    while (readdir(dir) != NULL) {
        if(++n > 2)
            break;
    }
    closedir(dir);
    if (n <= 2)             /* Directory Empty */
        return 1;
    else
        return 0;
}

int wu_clear_dir(const char* dir) {
    char path[WC_MAX_PATH];
    char temp[WC_MAX_PATH];

    if(!dir || !strlen(dir)) return 1; /* Nothing to delete */

    add_slash(temp, sizeof(temp), dir);

    snprintf(path, sizeof(path)-1, "%s%s%s", "rm -rf ", temp, "*");

    return (system(path) == 0);
}

pid_t wu_start_process(const char* prg_name, char* const* command_string, const char* process_working_dir) {
    pid_t pid = 0;
    assert(prg_name);

    if((pid = fork())) {
        return pid;     /* Parent return */
    }
    else {
        if(process_working_dir) chdir(process_working_dir);
        execv(prg_name, command_string);  /* launcher exits and disapears... */
    }
    return pid;
}

int wu_move_files(const char* dest_folder, const char* src_folder) {
    DIR           *d;
    struct dirent *dir;
    int ret = 1;

    if(d = opendir(src_folder), d) {
        char buf_src[WC_MAX_PATH];
        char buf_dst[WC_MAX_PATH];

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

int wu_move_n_rename(const char* old_dir, const char* old_name, const char* new_dir, const char* new_name) {
    char new_path[WC_MAX_PATH];
    char old_path[WC_MAX_PATH];
    char new_in_process[WC_MAX_PATH];

    wu_create_file_name(new_path, sizeof(new_path), new_dir, new_name, "");
    wu_create_file_name(old_path, sizeof(old_path), old_dir, old_name, "");
    wu_create_file_name(new_in_process, sizeof(new_in_process), new_dir, new_name, WUD_DEFAULT_FW_COPY_EXT);

/* Open old file */
    FILE* f_src = fopen(old_path, "rb");
    if(!f_src) {
        pu_log(LL_ERROR, "wu_move_n_rename: can't open file %s: %d, %s", old_path, errno, strerror(errno));
        goto on_err;
    }
/*Open new file with "in process" extention */
    FILE* f_cpy = fopen(new_in_process, "wb");
    if(!f_cpy) {
        pu_log(LL_ERROR, "wu_move_n_rename: can't open file %s: %d, %s", new_in_process, errno, strerror(errno));
        goto on_err;
    }
/* Coppy from src to cpy and flush at the end */
    unsigned char buf[4096];
    while(!feof(f_src)) {
        size_t ret = fread(buf, 1, sizeof(buf), f_src);
        if(ret) fwrite(buf, 1, ret, f_cpy);
    }
    if(fflush(f_cpy) != 0) {
        pu_log(LL_ERROR, "wu_move_n_rename: error copying file %s into %s: %d, %s", old_path, new_in_process, errno, strerror(errno));
        goto on_err;
    }
    fclose(f_src); f_src = NULL;
    fclose(f_cpy); f_cpy = NULL;
    pu_log(LL_DEBUG, "File %s copied into %s", old_path, new_in_process);
/* Delete old file */
    if(remove(old_path) != 0) {
        pu_log(LL_ERROR, "wu_move_n_rename: can't delete file %s:: %d, %s", old_path, errno, strerror(errno));
        goto on_err;
    }
/* Rename new file to the correct name (w/o extention) */
    if(rename(new_in_process, new_path)) {
        pu_log(LL_ERROR, "wu_move_n_rename: can't rename %s to %s: %d, %s", new_in_process, new_path, errno, strerror(errno));
        goto on_err;
    }
    pu_log(LL_DEBUG, "File %s renamed to %s", new_in_process, new_path);
    return 1;
on_err:
    if(f_src) fclose(f_src);
    if(f_cpy) fclose(f_cpy);
    return 0;
}

const char* wu_cut_off_file_name(const char* path_or_url) {
        if((!path_or_url) || !strlen(path_or_url)) return "";
        size_t i;
        for(i = strlen(path_or_url); i > 0; i--)
            if((path_or_url[i-1] == '/') || (path_or_url[i-1] == ':')) return path_or_url + i;
        return path_or_url;
}

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

const char* wu_create_file_name(char* buf, size_t buf_size, const char* dir, const char* fname, const char* ext) {
    assert(buf); assert(dir); assert(fname); assert(ext);

    add_slash(buf, buf_size, dir);
    strncat(buf, fname, buf_size-1-strlen(buf));
    if(strlen(ext)) {
        strncat(buf, ".", buf_size-1-strlen(buf));
        strncat(buf, ext, buf_size-1-strlen(buf));
    }
    return buf;
}

/****************************************
    Local finction implementation
*/

static const char* add_slash(char* buf, size_t buf_size, const char* path) {
    strncpy(buf, path, buf_size-1);
    if(buf[strlen(buf)-1] != '/')
        strncat(buf, "/", buf_size - strlen(buf) - 1);

    return buf;
}
