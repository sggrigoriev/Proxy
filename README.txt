Release 1.2.9

1. CMakeLists.txt defines
    These applications could be made for several platforms.
    Environment variables:
        PLATFORM =
            "cross"     Cross-compiler for OpenWRT
            "ubuntu"    Ubuntu
            "openwrt"   OpenWRT
            "quark"     Quark
            "hisilicon" Hisilicon (IPCamTenvis)
        LD_LIBRARY_PATH =
            list of paths to system libraries (Linux standard env variable).
            Set differently for each platform
        SDK_PATH =
            path to the Presto common lib (/lib) directory
        NO_GST_PLAGINS = - IPCamTenvis setting
            1 - add gstreamer plugins (hisilicon build)
            0 - ignore gstreamer plugins (ubuntu build)
    CMake defines:
        Proxy:
            LIBHTTP_CURL_DEBUG          - enable curl trace
            DEFAUT_U_TO                 - nanosleep in milliseconds
            DEFAULT_TO_RPT_TIMES        - amount of nanosleeps
                DEFAULT_U_TO * DEFAULT_TO_RPT_TIMES = pause before to set the new long GET in case of timeout
            DEFAULT_S_TO                - alternate timeout for the same purpose
            PROXY_AUTH_GET_EMUL         - Obsolete.
            GIT_COMMI
            GIT_BRANCH
            GIT_URL
            BUILD_DATE
            UNCOMMITED_CHANGES          - all these are used to print into Proxy log the current build info
            PROXY_ETHERNET_INTERFACE    - defines the interface uesed to send to the cloud the GW IP address used
            PROXY_ON_HOST               - if defined the Proxy could run by hands, not from WUD. Made for debugging purposes
            __LINUX__                   - needed for IPCam external library "ipcam"
            EXTERN=                     - needed for IPCam external library "ipcam"
        WUD:
            DEFAUT_U_TO                 - same as for Proxy
            DEFAULT_TO_RPT_TIMES        - same as for Proxy
            DEFAULT_S_TO                - same as for Proxy
            GIT_COMMIT                  - same as for Proxy
            GIT_BRANCH                  - same as for Proxy
            GIT_URL                     - same as for Proxy
            BUILD_DATE                  - same as for Proxy
            UNCOMMITED_CHANGES          - same as for Proxy
            WUD_NOT_STARTS_AGENT        - Agent should be started by hands. Used for M3 and for debugging
            WUD_ON_HOST                 - defined for start under ubuntu in debugging purposes
            PLATFORM_HISILICON_C1       - if defined, reboot(RB_AUTOBOOT), if not - reboot(RB_POWER_OFF)

2. WUD & Proxy configuration files.
    Common settings same for WUD & Proxy. Used ib both configuration files:
        LOG_NAME                    - log file name with path
        LOG_REC_AMT                 - log file records amount until the rewrite
        LOG_LEVEL                   - logging level. Possible values are from high to low:
            "ERROR", "INFO", "WARNING", "DEBUG", "TRACE_1", "TRACE_2"
        QUEUES_REC_AMT              - max amount of records stored in a queue. If the amount exceeds, the oldest will be deleted
        CURLOPT_CAINFO              - cURL specific
        CURLOPT_SSL_VERIFYPEER      - cURL specific
        CLOUD_CONN_TIMEOUT          - cURL connection timeout in seconds
        CLOUD_POST_ATTEMPTS         - cURL POST attempts before the error reported
        CLOUD_KEEP_ALIVE_INTERVAL   - cURL keep alive on TCP level sendings interval in seconds
        AGENT_PROCESS_NAME          - Printable process name to use in logging
        PROXY_PROCESS_NAME

    WUD specific
        WUD_WORKING_DIRECTORY       - path to the default WUD directory
        WUD_COMM_PORT               - WUD TCP port number
        CHILDREN_SHUTDOWN_TO_SEC    - pause before childs shutdown
            OTA settings:
        FW_DOWNLOAD_FOLDER          - path to the directory for firmware download
        FW_UPGRADE_FOLDER           - path to the directory with ready to use firmware
        FW_UPGRADE_FILE_NAME        - FW upgrade file name
            Child processes management settings:
        AGENT_BINARY_NAME           - Process's file name with full path
        AGENT_RUN_PARAMETERS        - parameters list surrounded square brackets: [par1, par2, ...]
        AGENT_WORKING_DIRECTORY     - full path to process workung directory
        AGENT_WD_TIMEOUT_SEC        - timeout for watchdog alert for the process
        PROXY_BINARY_NAME
        PROXY_RUN_PARAMETERSP
        ROXY_WORKING_DIRECTORY
        PROXY_WD_TIMEOUT_SEC
            Misc:
        WUD_MONITORING_TO_SEC       - not in use because non-working monitoring function
        WUD_DELAY_BEFORE_START_SEC  - time to let Proxy & Agent start
        REBOOT_BY_REQUEST           - 0 - make exit, 1 - make reboot
        CLOUD_FILE_GET_CONN_TO      - cURL connection parameter for file download

    Proxy specific:
        DEVICE_ID                   - used for debug in purpose to have fixed device ID w/o its generation
        AUTH_TOKEN_FILE_NAME        - full path and file name of "one string" file with auth_token
        MAIN_CLOUD_URL_FILE_NAME    - full path and file name of "one string" file with mail URL
        AGENT_PORT                  - TCP port to communicate with Agent
        WUD_PORT                    - TCP port to communicate with WUD
        DEVICE_TYPE                 - 7000 for camera, 31 for M3 currently
        CLOUD_URL_REQ_TO_HRS        - period to ask cloud for main URL change for device
        FW_VER_SEND_TO_HRS          - period to send device's firmware version
        WATCHDOG_TO_SEC             - period to send watchdog to WUD (NB! should be less than PROXY_WD_TIMEOUT_SEC)
        SET_SSL_FOR_URL_REQUEST     - 0 use http, 1 - use https. 0 used in debugging purposes only to have network trace
        DEVICE_ID_PREFIX            - Any string.
        REBOOT_IF_CLOUD_REJECTS     - Used for auth_token (re-)request from the cloud.
                                      0: continue requesting (M3)
                                      1: reboot (IPCamera)
        LONG_GET_TO                 - GET request timeout in seconds. Better to have around 2 min
        ALLOWED_DOMAINS             - list of domains allowed for device. If the list empty - all domains are Ok.
                                      List of strings in square brackets: ["url1", "url2", ...] or just []



