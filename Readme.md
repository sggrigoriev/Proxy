VERSION 1.1.6 01-APR-2017
=================================================================================================
1. Define PROXY_DEVICE_ID_PREFIX="aioxGW-" added for Proxy (C)make file to be able change the gatewayID prefix w/o code change
2. "Reboot" notification added:
    {"measures": [{"deviceId": "<deviceID>","params": [{"name": "reboot", "value": "<N>"}]}]}
   - V = 1 if the gateway is going to be rebooted by any reason (including the command from the cloud)
   - V = 2 on gateway's startup - i.e. after the reboot
3. Notification for main URL change by the gateway:
    {"measures": [{"deviceId": "<deviceID>", "params": [{"name": "cloud", "value": "<new cloud URL>"}]}]}
    The order is:
    - Gateway get new main URL
    - Gateway get new contact URL & new auth token
    - Gateway switches on new contact URL
    - Gateway sends the notification by old contact URL in he case of succesful switch to new URL.
4. Two new files appears in Proxy/proxy_threads directory:
    pt_change_cloud_notification.h
    pt_change_cloud_notification.c
5. Dates of releases for 2017 are fixed (2016 -> 2017)
6. Intel Quark support in Makefile.make
   to build directly on quark SoC HOST=intel-quark make -f Makefile.make

