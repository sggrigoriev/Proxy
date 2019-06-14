Release 1.2.9

WUD/wud_threads

    All WUD threads are here

    1. wt_agent_proxy_read -    Thread reads in async mode messages from Agent & Proxy by TCP
                                It works OK only if all connections (currently 2) are established!

    2. wt_fw_upgrade -          Firmware upgrade thread.
                                    - Download file.
                                    - Check sha256.
                                    - Copy to install directory, rename, delete source.
                                    - Finish - reboot.

    3. wt_monitor -             Monitor thread.
                                WUD does not rut it currently because the monitoring service is not implemented.

    4. wt_queues -              WUD queue manager. Should be shifted from wid_threads directory.

    5. wt_server_write -        Receives messages from WUD threads and writes 'em to the cloud.
                                Queue -> HTTP wrapper.

    6. wt_threads -             Main WUD loop. Contains all WUD threads startup with queues & tcp as well

    7. wt_watchdog -            WUD watchdog  thread.
                                Send alarm to request processor if one of children didn't send the watchdog message on time
