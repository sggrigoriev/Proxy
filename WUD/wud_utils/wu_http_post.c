//
// Created by gsg on 01/12/16.
//
#include <string.h>

#include "wu_http_post.h"

//Post to the cloud alerts
//Return out from cloud if smth wrong
//Return 1 if ok, 0 if error
int wu_http_post(const char* in, char* out, size_t out_len) {
    strncpy(out, "Answer from the cloud", out_len);
    return 1;
}

