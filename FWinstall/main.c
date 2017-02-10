//
// Created by gsg on 10/02/17.
//

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>


#include "pu_logger.h"
#include "lib_http.h"
#include "presto_release_version.h"

#define FW_INSTALL_NAME "FWinstall"

#define FW_INSTALL_RX_FNAME "./test.zip"
#define FW_INSTALL_URL "http://prod.fw.s3-website-us-east-1.amazonaws.com/test.zip"
#define FW_INSTALL_LOG_FILE "./FW_INSTALL_LOG"

#define FW_INSTALL_CONN_TO_SEC  60

int main(void) {
    int rc = -1;

    printf("Presto installer v %s\n", PRESTO_FIRMWARE_VERSION);

    pu_start_logger(FW_INSTALL_LOG_FILE, 100, LL_DEBUG);

    FILE* rx_fd = fopen(FW_INSTALL_RX_FNAME, "wb");
    if(!rx_fd) {
        pu_log(LL_ERROR, "%s: can't open %s file: %d, %s", FW_INSTALL_NAME, FW_INSTALL_RX_FNAME, errno, strerror(errno));
        goto on_finish;
    }
    if(!lib_http_init(1)) {
        pu_log(LL_ERROR, "%s: can't create HTTP connections pool", FW_INSTALL_NAME);
        goto on_finish;
    }
    lib_http_conn_t conn = lib_http_createConn(LIB_HTTP_FILE_GET, FW_INSTALL_URL, NULL, NULL, FW_INSTALL_CONN_TO_SEC);
    if(conn < 0) {
        pu_log(LL_ERROR, "%s: can't create HTTP connection to receive firmware", FW_INSTALL_NAME);
        goto on_finish;
    }
    while(1) {
        switch(lib_http_get_file(conn, rx_fd)) {
            case 1: // Got it!
                pu_log(LL_INFO, "%s: download of %s succeed... And nobody beleived!", FW_INSTALL_NAME, FW_INSTALL_RX_FNAME);
                goto on_finish;
            case 0: //timeout... try again and again, until the death separates us
                sleep(1);
                fclose(rx_fd);
                FILE* rx_fd = fopen(FW_INSTALL_RX_FNAME, "wb");
                if(!rx_fd) {
                    pu_log(LL_ERROR, "%s: can't reopen %s", FW_INSTALL_NAME, FW_INSTALL_RX_FNAME);
                    goto on_finish;
                }
                break;
            case -1:    //Error. Get out of here. We can't live in such a dirty world!
                pu_log(LL_ERROR, "%s, can't dowload the %s. Maybe in the next life...", FW_INSTALL_NAME, FW_INSTALL_RX_FNAME);
                goto on_finish;
        }
    }
on_finish:
    lib_http_close();
    if(rx_fd) fclose(rx_fd);
    exit(rc);

}
