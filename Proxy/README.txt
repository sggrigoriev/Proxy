The folder contains the Proxy part of Presto Gateway implementation for Jupiter project

Currently the proxy could communicate with the sandbox, receive commands translate it as is ot the Agent, receive agent's messages and
translate'em as is to the sandbox

proxyJSON.conf fields description

LOG_NAME                    path with Proxy log file. NB! File should be created for the first run
LOG_REC_AMT                 MAX records amount in log file (temp: SYSLOG will be used instaed)
LOG_LEVEL                   logging level. Possible variants are DEBUG, INFO, WARNING, ERROR


SERTIFICAE_PATH             temp setting - needed to play in sandbox. Path for private part of sertificate
ACTIVATION_TOKEN            currently copied from sandbox - activation procedure is not finalized
DEVICE_ADDRESS              set by hands in sandbox and copied here. Proxy can create old fasioned eui64. It should be modofied with the activation process
DEVICE_TYPE                 31 as Dmitriy sad
CLOUD_URL                   cloud url for routine communications. Currently set as a sandbox
MAIN_CLOUD_URL              default cloud contact point for the initial communication. Not used now

UPLOAD_TO_SEC               LONG GET timeout
QUEUES_REC_AMT              Max amount of records in a queue. If the max size is reached the oldest messages are shifted to the log and removed
AGENT_PORT                  port# to communicate with Agent


