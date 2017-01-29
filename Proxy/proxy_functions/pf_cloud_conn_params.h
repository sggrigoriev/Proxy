//
// Created by gsg on 22/11/16.
//
//Contains funcrions & data to make the proxy deviceID
// and perform the proxy activation on the Cloud

#ifndef PF_CLOUD_CONN_PARAMETERS_H
#define PF_CLOUD_CONN_PARAMETERS_H

//pf_get_cloud_conn_params   - get contact point URL & activation token from the cloud
//
// Actions:
//1. Get ProxyDeviceID
//2. Get cloud connection parameters:
//Return 0 if error and 1 if OK
int pf_get_cloud_conn_params();  //return 0 if proxy was not activated


#endif //PF_CLOUD_CONN_PARAMETERS_H
