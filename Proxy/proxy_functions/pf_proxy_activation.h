//
// Created by gsg on 22/11/16.
//
//Contains funcrions & data to make the proxy deviceID
// and perform the proxy activation on the Cloud

#ifndef PRESTO_PF_PROXY_ACTIVATION_H
#define PRESTO_PF_PROXY_ACTIVATION_H

//Check the presence of deviceID, activationKey and permanent cloudURL.
//If any of thiese is not found -
int pf_proxy_activation();  //return 0 if proxy was not activated

//Requests the CLOUD by defauld url for permanent contact URL
//NB! the function could be called in activation part as well as from recurring routine (bad connection case)
int pf_get_cloud_url(char* url, size_t size);

#endif //PRESTO_PF_PROXY_ACTIVATION_H
