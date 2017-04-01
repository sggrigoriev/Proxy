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
* Created by gsg on 31/03/17.
*/
#include <pthread.h>
#include <string.h>

#include "ph_manager.h"
#include "pr_commands.h"

#include "pc_settings.h"
#include "pf_traffic_proc.h"
#include "pt_change_cloud_notification.h"

#define PT_THREAD_NAME "CHANGE_CLOUD_URL_NOTIFYER"
/***************************************************************
    Local data
*/
static pthread_t id;
static pthread_attr_t attr;
static volatile int ccn_stop;
static volatile int ccn_run = 0;        /* Indicates the thread is currently running */

/* Local functions declaration */
/**********************************************************************
 * Stop thread
 */
static void stop_change_cloud_notification();

/**********************************************************************
 * Set stop flag: async stop
 */
static void set_stop_change_cloud_notification();

/***************************************************************
 * Thread function
*/
static void* read_proc(void* params);

/***************************************************************
 * Public functions implementation
 */

/**********************************************************************
 * Start thread. If the thread was started before it will stop it and restart
 * @return  - 1 if OK, 0 if not
 */
int pt_start_change_cloud_notification() {
    if(ccn_run) stop_change_cloud_notification();

    pthread_attr_init(&attr);
    pthread_create(&id, &attr, &read_proc, NULL);

    return 1;
}
/***************************************************************
 * Local functions implementation
 */

/**********************************************************************
 * Stop thread
 */
static void stop_change_cloud_notification() {
    void *ret;

    set_stop_change_cloud_notification();
    pthread_join(id, &ret);
    pthread_attr_destroy(&attr);
}

/**********************************************************************
 * Set stop flag: async stop
 */
static void set_stop_change_cloud_notification() {
    ccn_stop = 1;
}

static void* read_proc(void* params) {
    ccn_stop = 0;
    ccn_run = 1;

/* Sending with reconnects loop */
    int out = 0;
    while(!ccn_stop && !out) {
        char resp[LIB_HTTP_MAX_MSG_SIZE];

        if (!ph_notify(resp, sizeof(resp))) {
            pu_log(LL_ERROR, "%s:Error sending. ", PT_THREAD_NAME);
            out = 0;
            sleep(1);   /* Just on case... ph_notify got all pauses inside */
        } else {
            pu_log(LL_INFO, "%s: Main URL change notification sent to cloud", PT_THREAD_NAME);
            out = 1;
        }
    }
    pu_log(LL_INFO, "%s is finished", PT_THREAD_NAME);
    ccn_run = 0;
    pthread_exit(NULL);
 }