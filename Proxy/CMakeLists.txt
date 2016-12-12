cmake_minimum_required(VERSION 3.5)

project(Proxy C)

set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/../build)

add_definitions( -DGIT_FIRMWARE_VERSION=301 )
#add_definitions( -DPROXY_SEPARATE_RUN )
#ifdef PROXY_SEPARATE_RUN - the proxy reads parameters from command line first
#else - the proxy reads all configuration from default configuration file


set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall") # -std=c9x -lc -Wall -Werror")

set(REQUIRED_LIBRARIES
        m
        pthread
        curl
        rt
        )

#all include paths
include_directories(
        proxy_functions
        proxy_threads
        proxy_config
        proxy_config/proxy_config_cli
        proxy_eui64
        ../lib/presto_commands
        ../lib/pt_tcp_utl
        ../lib/pu_logger
        ../lib/pc_config
        ../lib/pu_queues
        ../lib/libhttpcomm
        ../lib/json
)

#Presto
file(GLOB SOURCE_FILES
        main.c
        proxy_functions/*.c
        proxy_threads/*.c
        proxy_config/pc_settings.c
        proxy_config/proxy_config_cli/*.c
        proxy_eui64/eui64.c
        ../lib/presto_commands/*.c
        ../lib/pt_tcp_utl/*.c
        ../lib/pu_logger/*.c
        ../lib/pc_config/pc_config.c
        ../lib/pu_queues/*.c
        ../lib/libhttpcomm/*.c
        ../lib/json/*.c
        )

#Test client
#file(GLOB SOURCE_FILES_TEST_CLIENT
#        test_client/comm_client.c
#       proxy_threads/*.c
#       proxy_config/pc_settings.c
#        ../lib/pt_tcp_utl/*.c
#        ../lib/pu_logger/*.c
#        ../lib/pc_config/pc_config.c
#        ../lib/pu_queues/*.c
#        ../lib/libhttpcomm/*.c
#        ../lib/json/*.c
#        )
#Test server
#file(GLOB SOURCE_FILES_TEST_SERVER
#        test_server/comm_server.c
#        proxy_threads/*.c
#        proxy_config/pc_settings.c
#        ../lib/pt_tcp_utl/*.c
#        ../lib/pu_logger/*.c
#        ../lib/pc_config/pc_config.c
#        ../lib/pu_queues/*.c
#        ../lib/libhttpcomm/*.c
#        ../lib/json/*.c
#        )

#Test config
#file(GLOB SOURCE_FILES_TEST_CONFIG
#        proxy_config/*.c
#        proxy_config/proxy_config_cli/*.c
#        proxy_eui64/eui64.c
#        ../lib/pt_tcp_utl/*.c
#        ../lib/pu_logger/*.c
#        ../lib/pc_config/pc_config.c
#        ../lib/pu_queues/*.c
#        ../lib/json/*.c
#        )

#Test eui
#file(GLOB SOURCE_FILES_TEST_EUI
#        ../lib/json/*.c
#        proxy_config/pc_settings.c
#        proxy_eui64/*.c
#        ../lib/pu_queues/*.c
#        ../lib/pt_tcp_utl/*.c
#        ../lib/pu_logger/*.c
#        ../lib/pc_config/pc_config.c
#        )


add_executable(Proxy ${SOURCE_FILES})
#add_executable(comm_client ${SOURCE_FILES_TEST_CLIENT} proxy_threads/pt_wud_write.c proxy_threads/pt_wud_write.h)
#add_executable(comm_server ${SOURCE_FILES_TEST_SERVER})
#add_executable(pc_test ${SOURCE_FILES_TEST_CONFIG})
#add_executable(eui_test ${SOURCE_FILES_TEST_EUI})


target_link_libraries(Proxy ${REQUIRED_LIBRARIES})
#target_link_libraries(comm_client ${REQUIRED_LIBRARIES})
#target_link_libraries(comm_server ${REQUIRED_LIBRARIES})
#target_link_libraries(pc_test ${REQUIRED_LIBRARIES})
#target_link_libraries(eui_test ${REQUIRED_LIBRARIES})