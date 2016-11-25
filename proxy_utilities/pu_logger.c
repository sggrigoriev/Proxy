//
// Created by gsg on 29/10/16.
//
#include <time.h>
#include <sys/time.h>
#include <stdarg.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>

#include "pu_logger.h"

////////////////////////////////////////////////////////////
//Module data
static unsigned long DEFAULT_REC_AMOUNT = 2048;

static FILE* file;
static fpos_t start_position;
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
        case LL_DEBUG:      return "DEBUG";
        case LL_INFO:       return "INFO";
        case LL_WARNING:    return "WARNING";
        case LL_ERROR:      return "ERROR";
        default:            return "UNDEF";
    }
}
////////////////////////////////////////////////////////////
//pu_start_logger - iitiates work of logger utility
//
//file_name     - string with the full path to the log file
//rec_amount    - size of max amount of records
//log_level     - defines the level of logging. All recirds with the type higher than log level will be suppressed
void pu_start_logger(const char* file_name, unsigned rec_amount, log_level_t l_level) {
    if (pthread_mutex_init(&lock, NULL) != 0) {
        printf("\n LOGGER: mutex init failed\n");
    }
    pthread_mutex_lock(&lock);

        log_lvl = l_level;
        file = NULL;
        rec_amt = 0;
        max_rec = (!rec_amount) ? DEFAULT_REC_AMOUNT : rec_amount;

        if (file_name != NULL) file = fopen(file_name, "w+");
        if (file == NULL) {
            file = stdout;
            printf("\nLOGGER: Error opening log file %s : %d, %s\n", file_name, errno, strerror(errno));
            printf("\nLOGGER: gona use stdout stream instead\n");
        }
        fgetpos(file, &start_position);

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
    if(log_lvl < lvl) return;   //Suppress the message

    pthread_mutex_lock(&lock);
        char date[30];

        getData(date, sizeof(date));

        va_list argptr;
        va_start(argptr, fmt);
        vsnprintf(buf, sizeof(buf), fmt, argptr);
        va_end(argptr);

        if(rec_amt >= max_rec) {
            if(file) fsetpos(file, &start_position);
            rec_amt = 0;
        }
        rec_amt++;
        if(file) fprintf(file, "%s %s %s\n", date, getLogLevel(lvl), buf); fflush(file);
    pthread_mutex_unlock(&lock);
}