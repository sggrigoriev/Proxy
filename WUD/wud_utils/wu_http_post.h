//
// Created by gsg on 01/12/16.
//

#ifndef PRESTO_WU_HTTP_POST_H
#define PRESTO_WU_HTTP_POST_H

#include <stdlib.h>

//Post to the cloud alerts
//Return out from cloud if smth wrong
//Return 1 if ok, 0 if error
int wu_http_post(const char* in, char* out, size_t out_len);

#endif //PRESTO_WU_HTTP_POST_H
