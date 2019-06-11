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
    Created by gsg on 22/11/16.

    Contains funcrions & data to make the proxy deviceID
    Should be refactored
*/
#ifndef PF_CLOUD_CONN_PARAMETERS_H
#define PF_CLOUD_CONN_PARAMETERS_H

/**
 * Generates device_id if it is not set in configuration file and save it in separate file - startup action
 * The name is wrong
 *
 * @return  - 0 if error and 1 if OK
*/
int pf_get_cloud_conn_params();  

#endif /*PF_CLOUD_CONN_PARAMETERS_H */
