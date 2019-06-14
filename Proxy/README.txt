Release 1.2.9
The folder contains the Proxy part of Presto M3 Gateway and IPCam implementation.
Proxy is the main communication Cloud <-> Presto point

The Proxy communication scheme:
		| <-> cloud	- receives commads for itself & for Agent (IoT or IPCamera); sends back to the cloud Agent's info and own info
Proxy 	| <-> Agent - forwards cloud info to the IoT agent and vice versa
		| -> WUD - sends to the WUD cloud connection info and cloud's commands related to WUD

Proxy communications:
	Proxy -> Cloud
		request for authentication token
		request for cloud contact URL
		inform about own firmware version
		immediate ACK for any command received

	Cloud -> Proxy
		command to change main URL
		command for reboot
		command to start firmware upgrade
		answer with new contact URL
		answer with authentication token
		
	Proxy -> IoT Agent
		all commands from the cloud regarding the agent
		on/off line status regarding the cloud communications state
		reports the own deviceID
		answrers with online/offline status by IoT Agent request
		
	Iot Agent -> Proxy
		forwards all agent's messages to the cloud
		requests the Proxy connection status
		
	Proxy -> WUD
		send connection information
		forward firmware upgrade & reboot commands
		send watchdog messages

Proxy threads:
	main_thread			runs and stops all other Proxy hreads
						establishes gateway connection with the cloud
						processes Proxy commands from cloud
						dispatches cloud messages to Agent and WUD
						fowards Agent messages to the cloud
						sends watchdogs to WUD
						sends requests for contact URL update
						
	main_agent			manages agent_read and agent_write threads to achieve assychrounous interaction between Proxy and Agent processes
	
		agent_read			receives info from Agent and sends it to the main_thread
	
		agent_write			sends info from Proxy threads to the Agent.
		
	server_read			receives info from the cloud (long GET). 
						split info from the cloud to the Agent and Proxy parts and forwards it to the agent_write and main_therad correspondingly
						creates immediate answer for commands from the cloud and sends it to the cloud via POST
						
	server_write		sends info from Proxy & Agent to the cloud via POST
						adds head to the messages sent
						sends online/offline statuses to the agent_write
						sends answers from cloud (if any) to the ageny_write
						
	wud_write			sends to WUD process all info from Proxy
		
Folders:
	/proxy_config				interfaces for read/write from/to Proxy configuration file and common Proxy defaults
	/proxy_functions			small garbage can. Contains wrapper for deviceID generation, immediate answer for cloud command arrival, reboot fincion in case of hard error
	/proxy_http					http connections manager for Proxy
	/proxy_threads				all threads running under Proxy to implement assync interactions with Iot agent and WUD
	/test_client				the IoT agent emulator for debugging purposes. Could be run in absence of real IoT agent.
	/main.c						Proxy start function
	/presto_release_version.h	define the Proxy release version
		

