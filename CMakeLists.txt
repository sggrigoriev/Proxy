cmake_minimum_required(VERSION 3.6)

project(Presto C)

set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ../build)

add_definitions( -DPROXY_SEPARATE_RUN )
add_definitions( -DGIT_FIRMWARE_VERSION=301 )
#ifdef PROXY_SEPARATE_RUN - the proxy reads parameters from command line first
#else - the proxy reads all configuration from default configuration file


set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall") # -std=c9x -lc -Wall -Werror")

set(REQUIRED_LIBRARIES
        m
        pthread
        curl
        )

#all include paths
include_directories(
        Proxy/proxy_functions
        Proxy/proxy_utilities
        Proxy/proxy_threads
        Proxy/proxy_config
        Proxy/proxy_config/proxy_config_cli
        lib/pt_tcp_utl
        lib/pu_logger
        lib/pc_config
        lib/libhttpcomm
        lib/json
        lib/eui64

)

#Presto
file(GLOB SOURCE_FILES
        Proxy/main.c
        Proxy/proxy_functions/*.c
        Proxy/proxy_utilities/*.c
        Proxy/proxy_threads/*.c
        Proxy/proxy_config/pc_settings.c
        Proxy/proxy_config/proxy_config_cli/*.c
        lib/pt_tcp_utl/*.c
        lib/pu_logger/*.c
        lib/pc_config/*.c
        lib/libhttpcomm/*.c
        lib/json/*.c
        lib/eui64/eui64.c
        )

#Test client
file(GLOB SOURCE_FILES_TEST_CLIENT
        Proxy/test_client/comm_client.c
        Proxy/proxy_utilities/*.c
        Proxy/proxy_threads/*.c
        Proxy/proxy_config/pc_settings.c
        lib/pt_tcp_utl/*.c
        lib/pu_logger/*.c
        lib/pc_config/*.c
        lib/libhttpcomm/*.c
        lib/json/*.c
        )

#Test config
file(GLOB SOURCE_FILES_TEST_CONFIG
        Proxy/proxy_config/*.c
        Proxy/proxy_config/proxy_config_cli/*.c
        proxy_utilities/*.c
        lib/pt_tcp_utl/*.c
        lib/pu_logger/*.c
        lib/pc_config/*.c
        lib/json/*.c
        lib/eui64/eui64.c
        )

#Test eui
file(GLOB SOURCE_FILES_TEST_EUI
        lib/json/*.c
        Proxy/proxy_utilities/*.c
        Proxy/proxy_config/pc_settings.c
        lib/pt_tcp_utl/*.c
        lib/pu_logger/*.c
        lib/pc_config/*.c
        lib/eui64/*.c
        )


add_executable(Presto ${SOURCE_FILES})
add_executable(comm_client ${SOURCE_FILES_TEST_CLIENT})
add_executable(pc_test ${SOURCE_FILES_TEST_CONFIG})
add_executable(eui_test ${SOURCE_FILES_TEST_EUI})


target_link_libraries(Presto ${REQUIRED_LIBRARIES})
target_link_libraries(comm_client ${REQUIRED_LIBRARIES})
target_link_libraries(pc_test ${REQUIRED_LIBRARIES})
target_link_libraries(eui_test ${REQUIRED_LIBRARIES})