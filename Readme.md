Presto Proxy C SDK
======================================================

This software SDK works in conjunection with People Power Presto to cloud-enable IoT devices and protocols.

The Presto Device API, implemented by this C SDK, is designed for connecting low memory embedded hubs, gateways, WiFi routers, set-top boxes, and embedded end-devices to Presto. Once connected to Presto, these devices are instantly exposed through mobile apps, web consoles, and data analytics + rules engines that you can customize.

It includes several directories:
/lib	common functionality for the application. It could be built as a separate libraries
/Proxy	the proxy part funcions:
			the cloud - gateway communictions
			IoT's Agent and cloud communications 
			run 3rd-party bots for rules implementation (not implemented yet)
			
/WUD	processes manager. Functions are:
			monitors the Proxy & Agent are alive
			reboots the gateway by cloud's command or if some of processes is dead
			make firmware upgrade initiated from cloud
			monitors system resources (not implemented yet)
			
REQUIRED LIBRARIES:
	PTHERADS support
	cURL version up to 7.47.0 support
	m
	
CMakeFiles configurations.
	Minimum CMake version required is 2.8.0
	Before the run the cmake you the envorinment variable "PRESTO_PLATFORM" should be devined. Possible values are:
		"cross"	- to run build for the MIPS platform on the Ubuntu maschine
		"host" - to run build for Ubuntu on the Ubuntu machine
		"OpenWRT" - to run build for MIPS planform on MIPS hardware
		
After the build you'll have 3 executable files:
	Proxy		- Proxy executable
	WUD			- WUD executable
	comm_client	- Agent's emulator. It could be useful to debug communitations between Proxy and the cloud w/o real IoT agent.
	
There are several configuration files described below:
	1. proxyJSON.conf (JSON format) - this is the Proxy configuration file. It should have exactly this name and be placed on the same directory as Proxy executable file.

		"PROXY_PROCESS_NAME":	"Proxy",	-- used in log files and gateway internal commands & alerts
		"AGENT_PROCESS_NAME":	"Agent",	-- used for debugging purposes. Not used in the MT platform.
		"AUTH_TOKEN_FILE_NAME":	"/root/presto/Proxy/auth_token",    -- path to the file with the gateway authentication token. For the first connection the gateway doesn't have this file. After the registration process on the cloud, the Proxy creates the file and writes its authentication token there.
		"LOG_NAME":				"/root/presto/Proxy/proxy.log",    -- path to the Proxy log file. It will be created in case of absence
		"LOG_REC_AMT":			10000,		-- max amount of records in the log file. If the log contains max records amount it would starts from the beginning.
		"LOG_LEVEL":			"DEBUG",    -- Logging level. Possible values are "DEBUG", "WARNING", "INFO", "ERROR". I'd recommend to leave the level as "DEBUG" for testing time
		"QUEUES_REC_AMT":		1024,		-- records amount in each Proxy's internal queue. If the queue size is going to grow upper this limit the oldest records will be lost.
		"AGENT_PORT":			60110		-- the port for Proxy <-> Agent communications. 
		"WUD_PORT":				8887,		-- the port for Proxy -> WUD communications. Proxy sends cloud connection info and watchdog messages to WUD using this port. The value should be equal to "WUD_COMM_PORT" setting in wud.conf.
		"DEVICE_TYPE":			31,			-- obsolete. Proxy does not use it for now. Agent sends this parameter to the cloud by itself.
		"MAIN_CLOUD_URL":		"http://sbox.presencepro.com",    -- the URL Proxy using to get the contact URL for further common work. The URL could be changed by cloud's command. In this case the parameter will be updated by Proxy in the configuration file.
		"CLOUD_URL_REQ_TO_HRS":	24,			-- the period in hours for the contact URL re-request. Proxy is requesting the contact URL from the cloud once by the period.
		"FW_VER_SEND_TO_HRS":	24			-- the period in hours to send by Proxy to the cloud its current firmware version
		"UPLOAD_TO_SEC":		120,		-- the time in seconds to wait the info from the cloud by Proxy, so called "long get" timeout. After the timeout the Proxy reestablishes GET to the cloud's contact URL.
		"WATCHDOG_TO_SEC":		30			-- the timeout in seconds for watchdog sending to the WUD by Proxy. In real life the accuracy is about 1 second: the watchdog could be sent after 31 instead of 30 seconds.

	2. wud.conf (JSON format) - this is the WUD configuration file. It should have exactly this name and be placed with the same directory with WUD executable file.

		"LOG_NAME":                     "/root/presto/WUD/wud.log",    -- the path for WUD log file. It will be created in case of absence.
		"LOG_REC_AMT":                  5000,		-- max amount of WUD log file records. If the log contains max records amount it would starts from the beginning.
		"LOG_LEVEL":                    "DEBUG",	-- log level for WUD log file. Possible values as "DEBUG", "WARNING", "INFO", "ERROR". I'd recommend to leave the "DEBUG" level for testing time
		"QUEUES_REC_AMT":               1024,		-- max amount of internal WUD queues each. If the queue size is going to grow upper this limit the oldest records will be lost.
		"WUD_WORKING_DIRECTORY":        "/root/presto/WUD",    -- WUD working directory - the place for WUD executable and configuration file
		"WUD_COMM_PORT":                8887		-- WUD communication port. It should be the same as "WUD_PORT" in the Proxy configuration file.
		"CHILDREN_SHUTDOWN_TO_SEC":     10,			-- timeout for the graceful shutdown for Proxy and Agent. Graceful shutdown does not implemented now.
		"FW_DOWNLOAD_FOLDER":           "/root/download",    -- the folder WUD downloading the firmware file from the cloud
		"FW_UPGRADE_FOLDER":            "/root/upgrade",    -- the folder WUD moves the firmware file after check and rename. The file name should be "presto_mt7688.tgz" - this name is hard coded for now.
		"AGENT_PROCESS_NAME":           "zbagent",	-- the Agent process name. It was planned the WUD will start Proxy and Agent. Currently the WUD starts just Proxy.
		"AGENT_BINARY_NAME":            "/root/presto/zbagent/zbagent", -- the Agent's executable name. Not in use until the Agent starts by itself.
		"AGENT_RUN_PARAMETERS":         [],			-- same
		"AGENT_WORKING_DIRECTORY":      "/root/presto/zbagent",    -- same
		"AGENT_WD_TIMEOUT_SEC":         600,		-- timeout for watchdog messages from the Agent. If the watchdog from the Agent will not appear the WUD will reboot the gateway.
		"PROXY_PROCESS_NAME":           "Proxy",	-- should be the same as "PROXY_PROCESS_NAME" in the proxy configuration
		"PROXY_BINARY_NAME":            "/root/presto/Proxy/Proxy",
		"PROXY_RUN_PARAMETERS":         [],			-- since the WUD starts the Proxy it is possible to pass to the Proxy some start parameters. I used this to start Proxy under valgrind to check memory leaks. The format is JSON array:
		["param1", "param2", ...]
		"PROXY_WORKING_DIRECTORY":      "/root/presto/Proxy",
		"PROXY_WD_TIMEOUT_SEC":         600,		-- timeout for watchdog messages from the Proxy. If the watchdog from the Proxy will not appear the WUD will reboot the gateway.
		"WUD_MONITORING_TO_SEC":        3600,		-- not used until the system monitoring by WUD (available memory, file descriptors, etc) is done.
		"WUD_DELAY_BEFORE_START_SEC":    120		-- initial WUD delay during the gateway startup.

	3. auth_token - The file contains one string with gateway authentication token.

	4. firmware_release - The file contains the string with the firmware version. 
	