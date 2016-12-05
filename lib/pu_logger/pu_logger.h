//
// Created by gsg on 29/10/16.
//
// Contains all functions and data related to Proxy logging
// NB! Logging utility is thread-protected!
// The log has fixed amount of records. After the max records size is riched it starts rigth from the beginning

#ifndef PRESTO_PU_LOGGER_H
#define PRESTO_PU_LOGGER_H

#include <unistd.h>

typedef enum {LL_DEBUG = 3, LL_INFO = 2, LL_WARNING = 1, LL_ERROR = 0} log_level_t;

void pu_start_logger(const char* file_name, size_t rec_amount, log_level_t l_level);    //initiate logging utility
void pu_stop_logger();  // close log file
void pu_set_log_level(log_level_t ll);
void pu_log(log_level_t lvl, const char* fmt, ...);


#endif //PRESTO_PU_LOGGER_H
