The folder contains the Proxy part of Presto Gateway implementation for Jupiter project

WUD currently could run Proxy and Agent emulator. All other functions are under development

wud.conf fields description:

LOG_NAME                    path with WUD log file. NB! File should be created for the first run
LOG_REC_AMT                 MAX records amount in log file (temp: SYSLOG will be used instaed)
LOG_LEVEL                   logging level. Possible variants are DEBUG, INFO, WARNING, ERROR

WUD_WORKING_DIRECTORY       path to the WUD working directory
WUD_COMM_PORT               port# for listening Agent & Proxy
CHILDREN_SHUTDOWN_TO_SEC    timeout in seconds between SIGTERM & SIGKILL signals in case of termination

FW_DOWNLOAD_FOLDER          path to folder with FW upgrade files download
FW_UPGRADE_FOLDER           path to folder with FW upgrade files ready for installation

AGENT_PROCESS_NAME          agent process name to print in logs & alerts
AGENT_BINARY_NAME           path to Agent executable
AGENT_RUN_PARAMETERS        Agent programm run parameters or [] if no parameters
AGENT_WORKING_DIRECTORY     path to the Agent working directory
AGENT_WD_TIMEOUT_SE         timeout for Agent watch dog messages

PROXY_PROCESS_NAME          same Proxy parameters
PROXY_BINARY_NAME
PROXY_RUN_PARAMETERS
PROXY_WORKING_DIRECTORY
PROXY_WD_TIMEOUT_SEC




