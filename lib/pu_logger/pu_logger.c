//
// Created by gsg on 29/10/16.
//
#include <time.h>
#include <sys/time.h>
#include <stdarg.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <limits.h>

#include "pthread.h"
#include "pu_logger.h"

#define PU_LOG_REC_SIZE     160
#define PU_END_FILL ' '

////////////////////////////////////////////////////////////
//Module data
static unsigned long DEFAULT_REC_AMOUNT = 2048;

static FILE* file;

static unsigned long rec_amt;
static unsigned long max_rec;

static log_level_t log_lvl;
static pthread_mutex_t lock;

////////////////////////////////////////////////////////////
//Local functions
//
///////////////////////////////////////////////////////////
//getData - returns the string with current timestamp. Format: YY-MM-DD-HH24-MIM-SEC-MSEC
//
//buf       - returned buf with the date
//max_size  - buffer size
//
static char* getData(char* buf, unsigned max_size) {
    time_t rawtime;
    struct tm timeinfo;
    struct timeval tv;
    struct timezone tz;

    time (&rawtime);
    localtime_r (&rawtime, &timeinfo);
    gettimeofday(&tv, &tz);
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
///////////////////////////////////////////////////////////
//getLogLevel - returns the const string with log level name
//
static const char* getLogLevel(log_level_t lvl) {
    switch(lvl) {
        case LL_DEBUG:      return "DEBUG  ";
        case LL_INFO:       return "INFO   ";
        case LL_WARNING:    return "WARNING";
        case LL_ERROR:      return "ERROR  ";
        default:            return "UNDEF  ";
    }
}
// count the number of lines in the file called filename
static unsigned long countlines(const char *filename) {
    FILE *fp = fopen(filename,"r");
    int ch;
    unsigned long lines=0;

    if (fp == NULL) return 0;

    while(!feof(fp)) if(ch = fgetc(fp), ch == '\n') lines++;

    fclose(fp);
    return lines;
}
//Make the output line of PU_LOG_REC_SIZE-1 len. Returns NULL if o info
//NB! rest initially must set equal to strlen(in) and decreasing after each get_line call
static char out[PU_LOG_REC_SIZE];
static const char* get_line(const char* in, unsigned int offset, size_t* rest) {
    if(*rest < strlen(in)) {
        memset(out, ' ', offset);
    }
    else {
        offset = 0;
    }
    if(*rest > 0) {
        size_t info_size = (((*rest)+offset) > PU_LOG_REC_SIZE-1)?PU_LOG_REC_SIZE-1:((*rest)+offset);
        strncpy(out+offset, in+strlen(in)-(*rest), info_size);
        memset(out+info_size, PU_END_FILL, PU_LOG_REC_SIZE-1 - info_size);
        out[PU_LOG_REC_SIZE-1] = '\0';
        (*rest) -= (info_size-offset);
        return out;
    }
    return NULL;
}
////////////////////////////////////////////////////////////
//pu_start_logger - iitiates work of logger utility
//
//file_name     - string with the full path to the log file
//rec_amount    - size of max amount of records
//log_level     - defines the level of logging. All recirds with the type higher than log level will be suppressed
void pu_start_logger(const char* file_name, size_t rec_amount, log_level_t l_level) {
    if (pthread_mutex_init(&lock, NULL) != 0) {
        printf("\n LOGGER: mutex init failed\n");
    }
    pthread_mutex_lock(&lock);

        log_lvl = l_level;
        file = NULL;
        rec_amt = 0;

        rec_amt = countlines(file_name);

        if(!file_name) {
            max_rec = ULONG_MAX;
         }
        else {
            max_rec = (!rec_amount) ? DEFAULT_REC_AMOUNT : rec_amount;
            file = fopen(file_name, "r+");
            if(file) fseek(file, 0, SEEK_END);
         }
        if (file == NULL) {
            file = stdout;
            printf("\nLOGGER: Error opening log file %s : %d, %s\n", file_name, errno, strerror(errno));
            printf("\nLOGGER: gona use stdout stream instead\n");
        }

    pthread_mutex_unlock(&lock);
 }
////////////////////////////////////////////////////////////
//pu_stop_logger - stops work of logger utility
//
void pu_stop_logger() {
    if(file) (fclose(file), file = NULL);
    pthread_mutex_destroy(&lock);
}
////////////////////////////////////////////////////////////
//pu_set_log_level - chanfes log level from current to ll
//
void pu_set_log_level(log_level_t ll) {
    pthread_mutex_lock(&lock);
        log_lvl = ll;
    pthread_mutex_unlock(&lock);
}
////////////////////////////////////////////////////////////
//pu_log - write info to the log
//
//lvl       - level of info (LL_DEBUG, ... , LL_ERROR
//fmt, ...  - formatted output as in printf
//
static char buf[10000]; //to decrease the stack size...Anyway the buf access is under Lock protection
void pu_log(log_level_t lvl, const char* fmt, ...) {
    unsigned int offset;
     if(log_lvl < lvl) return;   //Suppress the message

    pthread_mutex_lock(&lock);
        size_t rest;

        getData(buf, sizeof(buf));
        snprintf(buf+strlen(buf), sizeof(buf)-strlen(buf)-1, " %s ", getLogLevel(lvl));
        offset = (unsigned int)strlen(buf);

        va_list argptr;
        va_start(argptr, fmt);
        vsnprintf(buf+strlen(buf), sizeof(buf)-strlen(buf)-1, fmt, argptr);
        va_end(argptr);

        rest = strlen(buf);
        while(rest) {
            if (rec_amt >= max_rec) {
                if (file) {
                    rewind(file);
                }
                rec_amt = 0;
            }
           if (file) {
                fprintf(file, "%s\n", get_line(buf, offset, &rest));
                fflush(file);
            }
            rec_amt++;
        }
    pthread_mutex_unlock(&lock);
}