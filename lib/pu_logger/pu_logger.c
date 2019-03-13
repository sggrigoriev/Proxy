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
 Created by gsg on 29/10/16.
*/
#include <time.h>
#include <sys/time.h>
#include <stdarg.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <limits.h>

#include "pthread.h"
#include "pu_logger.h"

/*
Module data
*/
/* Log file name */
static char LOG_FILE_NAME[4097] = "./default_log.log";
/* Log file descriptor */
static FILE* file;

/* Currently written records amount */
static unsigned long rec_amt;

/* Max records allowed in log file*/
static unsigned long max_rec;

/* Log level: logger will not print messages with log level greater than this*/
static log_level_t log_lvl;

/* Thread protection */
static pthread_mutex_t lock;

/*/
Local functions

/
*/
/*
 Get the system date & time. Format: YY-MM-DD-HH24-MIM-SEC-MSEC
  buf       - returned buf with the date
  max_size  - buffer size
 Return pointer to the buf
*/
static char* getData(char* buf, unsigned max_size) {
    time_t rawtime;
    struct tm timeinfo;
    struct timeval tv;
    /*mlevitin struct timezone tz;*/

    time (&rawtime);
    localtime_r (&rawtime, &timeinfo);
    /*gettimeofday(&tv, &tz);*/
    gettimeofday(&tv, NULL);
    snprintf(buf, max_size, "%02u-%02u-%02u %02u:%02u:%02u.%03ld",
             timeinfo.tm_year-100,
             timeinfo.tm_mon+1,
             timeinfo.tm_mday,
             timeinfo.tm_hour,
             timeinfo.tm_min,
             timeinfo.tm_sec,
             tv.tv_usec/1000
    );
    return buf;
}

/************************************************
 * Oepn log file. If error - set stdout as a stream
 * adding = 0 - truncate; adding = 1  add records
 * @return  - pointer to file descriptor
 */
static FILE* open_log(const char* fname, int append) {

    FILE* ret = fopen(fname, (append)?"a":"w");

    if (ret == NULL) {
        ret = stdout;
        printf("\nLOGGER: Error opening log file %s : %d, %s\n", fname, errno, strerror(errno));
        printf("\nLOGGER: gona use stdout stream instead\n");
    }
    return ret;
}

/*/
    Public functions
*/
void pu_start_logger(const char* file_name, size_t rec_amount, log_level_t l_level) {
    if (pthread_mutex_init(&lock, NULL) != 0) {
        printf("\n LOGGER: mutex init failed\n");
    }
    pthread_mutex_lock(&lock);

        log_lvl = l_level;
        file = NULL;
        rec_amt = 0;
        max_rec = rec_amount;
        strncpy(LOG_FILE_NAME, file_name, sizeof(LOG_FILE_NAME)-1);

        file = open_log(LOG_FILE_NAME, 1);

    pthread_mutex_unlock(&lock);
 }

void pu_stop_logger() {
    if(file) (fclose(file), file = NULL);
    pthread_mutex_destroy(&lock);
}

void pu_set_log_level(log_level_t ll) {
    pthread_mutex_lock(&lock);
        log_lvl = ll;
    pthread_mutex_unlock(&lock);
}

/* to decrease the stack size...Anyway the buf access is under Lock protection */
static char buf[10000];
void pu_log(log_level_t lvl, const char* fmt, ...) {
    pthread_mutex_lock(&lock);
     if(log_lvl < lvl){ pthread_mutex_unlock(&lock); return; }   /*Suppress the message*/
        char head[31];
        getData(head, sizeof(head));

        snprintf(buf, sizeof(buf)-1, "%s %lu %s ", head, pthread_self(), getLogLevel(lvl));

        size_t offset = strlen(buf);

        va_list argptr;
        va_start(argptr, fmt);
        vsnprintf(buf+offset, sizeof(buf)-offset-1, fmt, argptr);
        va_end(argptr);

        if (rec_amt >= max_rec) {
            fclose(file);
            file = open_log(LOG_FILE_NAME, 0);
            rec_amt = 0;
        }
        if (file != NULL)
            fprintf(file, "%s\n", buf),
            fflush(file);
        else
            fprintf(stderr, "%s\n", buf),
            fflush(stderr);

        rec_amt++;

    pthread_mutex_unlock(&lock);
}

/*
 Convert the log level to its name
      lvl - log level type
    Return the log level name as a string
*/
const char* getLogLevel(log_level_t lvl) {
    switch(lvl) {
        case LL_DEBUG:      return "DEBUG  ";
        case LL_INFO:       return "INFO   ";
        case LL_WARNING:    return "WARNING";
        case LL_ERROR:      return "ERROR  ";
        case LL_TRACE_1:    return "TRACE_1";
        case LL_TRACE_2:    return "TRACE_2";
        default:            return "UNDEF  ";
    }
}