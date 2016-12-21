//
// Created by gsg on 22/11/16.
//

#include <string.h>
#include <pt_http_utl.h>
#include <errno.h>
#include "pc_defaults.h"
#include "pu_logger.h"
#include "pc_settings.h"
#include "pf_proxy_activation.h"

static int get_activation_token(char* token, size_t size);

////////////////////////////////////////////////////////
//pf_proxy_activation   - analyze the presence of deviceID and activation token in loaded settings
//
// Possible actions:
//1. Do nothing if activated (both items are found)
//2. Get activation token from the cloud
//3. Generate deviceID and get the activation token from the cloud
//Return 0 if error and 1 if OK
//
int pf_proxy_activation() {  //return 0 if proxy was not activated
    if(!ps_existsProxyDeviceID()) {
        char eui_string[PROXY_DEVICE_ID_SIZE];
        if(!eui64_toString(eui_string, sizeof(eui_string))) {
            pu_log(LL_ERROR, "pf_proxy_activation: Unable to get the Gateway DeviceID. Activaiton failed");
            return 0;
        }
        if(!pc_saveProxyDeviceID(eui_string)) {
            pu_log(LL_WARNING, "pf_proxy_activation: Unable to store Device ID into configuration file");
        }
    }
    if(!pc_existsActivationToken()) {
        char a_token[PROXY_MAX_ACTIVATION_TOKEN_SIZE];
        if(!get_activation_token(a_token, sizeof(a_token))) {
            pu_log(LL_ERROR, "pf_proxy_activation: Unable to get Activaton Token. Activation failed");
            return 0;
        }
        if(!pc_saveActivationToken(a_token)) {
            pu_log(LL_WARNING, "pf_proxy_activation: Unable to store Activaton Token into configuration file");
        }
    }
    if(!pc_existsCloudURL()) {
        char contact_url[LIB_HTTP_MAX_URL_SIZE];
        if(!pf_get_cloud_url(contact_url, sizeof(contact_url))) {
            pu_log(LL_ERROR, "pf_proxy_activation: Unable to get permanent Cloud URL. Activation failed");
            return 0;
        }
        if(!pc_saveCloudURL(contact_url)) {
            pu_log(LL_WARNING, "pf_proxy_activation: Unable to store permanent Cloud URL into configuration file");
        }
    }
    return 1;
}

//Requests the CLOUD by defauld url for permanent contact URL
//NB! the function could be called in activation part as well as from recurring routine (bad connection case)
int pf_get_cloud_url(char* url, size_t size) {

    return 0;
}

static int get_activation_token(char* token, size_t size) {
    char resp[LIB_HTTP_MAX_MSG_SIZE];
    int ok = 0;

    do {    //we'll be here until the victory
        switch(pt_http_write("", 0, resp, sizeof(resp))) {  //put empty request
            case LIB_HTTP_POST_AUTH_TOKEN:
                strncpy(token, resp, size);
                token[size-1] = '\0';
                ok = 1;
                break;
            case LIB_HTTP_POST_ERROR: //ERR FORMAT
                pu_log(LL_ERROR,"get_activation_token: Format error. Retry. %s", resp);
                break;
            case LIB_HTTP_POST_RETRY: //ERR: conn problem
                pu_log(LL_ERROR, "get_activation_token: Transmit error. Retry. %s.", resp);
                break;
            case LIB_HTTP_POST_OK:  //ACK or CONT - unapplicable in our case
                pu_log(LL_ERROR, "get_activation_token: Wrong expected status. Retry. %s", resp);
                break;
            default:            // Other bugs
                pu_log(LL_ERROR, "get_activation_token: Error %s - %d, %s", resp, errno, strerror(errno));
                break;
        }

    } while (ok);

    return 1;
}