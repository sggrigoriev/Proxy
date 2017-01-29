//
// Created by gsg on 13/11/16.
//
// Contains data and functions to communicate with the Cloud

#ifndef PH_MANAGER_H
#define PH_MANAGER_H

#include "lib_http.h"
//Case of initiation connections:
//1. Get main url & deviceId from config
//2. Get contact url from main
//3. If no auth_token, get auth token
//4. Open connections
// If error - start from 1.
void ph_mgr_start();        //Returns 0 if failed 1 if succeed
void ph_mgr_stop();

//Case when the main url came from the cloud
//1. Get contact url from main
//2. Get auth token
//2. Reopen connections
//3. save new main & new contact & auth token
//if error - close evrything and make the step "initiation connections"; return 0
int ph_update_main_url(const char* new_main);

//Case of connection error
//1. Get contact url
//2. Reopen connections
//3. if error start from 1
//4. save new contact url
void ph_reconnect();

//Case of periodic update the contact url
//1. Get contact url from main url
//2. Reopen connections
//If error - open connections with previous contact url
//If error again - start from step 1
//3. Save new contact url
void ph_update_contact_url();

// Communication functions
//
// 0 if TO, -1 if conn error, 1 if OK
int ph_read(char* in_buf, size_t size);

//Returns 0 if error, 1 if OK
int ph_write(char* buf, char* resp, size_t resp_size);

#endif //PH_MANAGER_H
