The folder contains the WUD part of Presto Gateway implementation.

The WUD communication scheme:
	| -> cloud - sends alerts and firmware upgrade statuses
WUD	| <- Proxy - receives connection info, firmware upgrade command, reboot command, watchdog
	| <- Agent - receives watchgogs
	
WUD communications
	WUD -> cloud	
		firmware upgrade status: STOP, START, IN_PROCESS
		alerts: PROXY_REBOOT
	Proxy->WUD
		cloud connection info
		watchdog message
		firmware upgrade command
		reboot command
		
	Agent->WUD
		wiatchdog message
		
WUD threads:
	request_processor	manage all WUD threads
						process Proxy commands & alerts from own threads
						reboot the gateway
							
	agent_proxy_read	read info from Agent & Proxy
						forwards the info to the request_processor
						
	fw_upgrade			implements the firmware upgrade process:
							read the firmware file from the site pointed by cloud
							check the file check sum versus the check sum sent by cloud
							move (copy&delete) file to the installation folder
							rename the file to the dedicated name
							send notification to the cloud (server_write thread) and to the request_processor
							
	server_write		send to cloud the info form WUD
	
	watchdog			check the watchdog timers of Proxy & Agent
						send to request_processor the timeout alarm if the timer exceeds
						
Folders:
	/wud_actions		watchdog service for Proxy & Agent; Proxy & Agent run/restart/stop services; Gateway reboot function
	/wud_config			interfaces for read from WUD configuration file and common WUD defaults
	/wud_fw_upgrade		helpers for firmware upgrade process
	/wud_http			http connections manager for WUD
	/wud_threads		all threads running under WUD
	/wud_utils			wrappers for some Linux system calls 
	/main.c				WUD start function
					



