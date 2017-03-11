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
// Created by gsg on 29/10/16.
//
// Contains all functions and data related to Proxy logging
// NB! Logging utility is thread-protected!
// The log has fixed amount of records. After the max records size is riches it starts right from the beginning
*/
#ifndef PRESTO_PU_LOGGER_H
#define PRESTO_PU_LOGGER_H

#include <unistd.h>

typedef enum {LL_DEBUG = 3, LL_INFO = 2, LL_WARNING = 1, LL_ERROR = 0} log_level_t;

/* Initiate logging utility. If there are some problems with file_name file opening, the logger will use stdout stream instead
 *  file_name   - log file name with path. Use NULL to have stdout
 *  rec_amount  - max records amount allowed in log file
 *  l_level     - log output level.
*/
void pu_start_logger(const char* file_name, size_t rec_amount, log_level_t l_level);

/* Close the lof file and stop logging
 *
*/
void pu_stop_logger();

/* Update the log level
 *  ll  - new log level
*/
void pu_set_log_level(log_level_t ll);

/* Print the info into the log file in standard printf-a like format
 *  lvl     - level of info (DEBUG, INFO,...)
 *  fmt     - formatted message
*/
void pu_log(log_level_t lvl, const char* fmt, ...);


#endif //PRESTO_PU_LOGGER_H
