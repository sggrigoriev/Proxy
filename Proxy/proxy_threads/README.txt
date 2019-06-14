Release 1.2.9

Proxy/proxy_threads
    1. pt_agent_read -  Wrapper thread: wait for messages from Agent and passes it to the Proxy though queue.
                        Mainly TCP -> queue forwarder.

    2. pt_agent_write - Wrapper thread: wait for messages for Agent and passes it to the Agent trough TCP.
                        Mainly queue -> TCP forwarder.

    3. pt_main_agent -  The manager read/write threads: wait for Agent start,
                        reconnect read/write threads in case of Agent fails.

    4. pt_queues -      Proxy queues manager. Better to move it to the separate folder.

    5. pt_reconnect -   Short live thread just to fulfill cloud command to change main URL.

    6. pt_server_read - Read data from the cloud (long GET) and send to main proxy and to the agent write threads.
                        Mainly the HTTP(s)-> queue wrapper.

    7. pt_server_write -Forwards info from to the cloud. Sends answer to the agent write thread.
                        Mainly the wrapper for queue -> HTTP(s).
                        In theory some of answers are for Proxy (if Proxy sent some commands) but it is not important for now.

    8. pt_threads -     Main Proxy loop and init/close/run/stop for all Proxy threads.

    9. pt_wud_write -   Thread for Proxy -> WUD async communications.
                        The wrapper for queue -> TCP.
                        There is no WUD -> Proxy info channel yet.
