Release 1.1.4
Presto/lib - common functionality for the application. It could be built as a separate libraries.

/json				3rd-party library (Copyright (c) 2009 Dave Gamble) for JSON strings parsing and JSON strings creation
/libhttpcomm		wrapper under cURL for gateway communication with the cloud via http(s) protocol
/libsha				3rd-party library (https://github.com/B-Con/crypto-algorithms/blob/master/) adopted by local needs for SHA256 calculation
/libtcp				wrapper under standard tcp Linux interface
/libtimer			gateway time service for timeouts and periodical actions start. Works with server uptime as base because of funny time jumps during the GW initiation
/pc_config			wrappper under JSON library for basic operations with comfiguration file
/presto_commands	interface for all cloud <-> gateway commands parsing and creation. Currently looks like the garbage can. Also contains some helpers.
/pu_logger			gateway logging funcions
/pu_queues			gateway internal inter-thread communication mechanism.

NB! some libraries are dependant on pu_logger library.