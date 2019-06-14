Release 1.2.9
Presto/lib - common functionality for Proxy, WUD, IPCam, M3-Agent. It could be built as a separate libraries.
+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
/json
    3rd-party library (Copyright (c) 2009 Dave Gamble) for JSON strings parsing and JSON strings creation

/libhttpcomm
    Wrapper under cURL for gateway communication with the cloud via http(s) protocol
    Depends on pu_logger library!
    No thread protection!

/libsha
    3rd-party library (https://github.com/B-Con/crypto-algorithms/blob/master/).
    Adopted for local needs for SHA256 calculation.

/libtcp
    Wrapper under standard tcp Linux interface for async read.
    All messages for read & write should be 0-terminated strings!
    Interface functions are thread-protected.
    Depends on pu_logger library!

/libtimer
    Gateway time service for timeouts and periodical actions start.
    Works with server uptime as base because of funny time jumps during the GW initiation
    Depends on pu_logger library!

/pc_config
    Wrappper under JSON library for basic operations with configuration file
    Used for proxyJSON.conf, wud.conf, Tenvis.conf read as a base.

/presto_commands
    Interface for all cloud <-> gateway commands parsing and creation.
    Currently is a the garbage can and has to be refactored.
    Depends on pu_logger library!

/pu_logger
    Common logging functionality for Proxy/WUD/IPCam.
    Thread-protected
    The log has fixed amount of records.
    After the max records size is riches it starts right from the beginning

/pu_queues
    Internal inter-thread communication mechanism for Proxy/WUD/IPCam.
    Circular thread protected queue implementation. Multiple async reads/writes allowed.
    Data element is char. 0-termination is not required
    Depends on pu_logger library!


