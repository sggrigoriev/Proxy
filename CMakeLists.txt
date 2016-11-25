cmake_minimum_required(VERSION 3.6)

project(Presto C)

set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/build)

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
        ${PROJECT_SOURCE_DIR}/proxy_functions
        ${PROJECT_SOURCE_DIR}/proxy_utilities
        ${PROJECT_SOURCE_DIR}/proxy_threads
        ${PROJECT_SOURCE_DIR}/proxy_config
        ${PROJECT_SOURCE_DIR}/proxy_config/proxy_config_cli
        ${PROJECT_SOURCE_DIR}/lib/libhttpcomm
        ${PROJECT_SOURCE_DIR}/lib/json
        ${PROJECT_SOURCE_DIR}/lib/eui64

)

#Presto
file(GLOB SOURCE_FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/main.c
        ${CMAKE_CURRENT_SOURCE_DIR}/proxy_functions/*.c
        ${CMAKE_CURRENT_SOURCE_DIR}/proxy_utilities/*.c
        ${CMAKE_CURRENT_SOURCE_DIR}/proxy_threads/*.c
        ${CMAKE_CURRENT_SOURCE_DIR}/proxy_config/pc_settings.c
        ${CMAKE_CURRENT_SOURCE_DIR}/proxy_config/proxy_config_cli/*.c
        ${CMAKE_CURRENT_SOURCE_DIR}/lib/libhttpcomm/*.c
        ${CMAKE_CURRENT_SOURCE_DIR}/lib/json/*.c
        ${CMAKE_CURRENT_SOURCE_DIR}/lib/eui64/eui64.c
        )

#Test client
file(GLOB SOURCE_FILES_TEST_CLIENT
        ${CMAKE_CURRENT_SOURCE_DIR}/test_client/comm_client.c
        ${CMAKE_CURRENT_SOURCE_DIR}/proxy_utilities/*.c
        ${CMAKE_CURRENT_SOURCE_DIR}/proxy_threads/*.c
        ${CMAKE_CURRENT_SOURCE_DIR}/proxy_config/pc_settings.c
        ${CMAKE_CURRENT_SOURCE_DIR}/lib/libhttpcomm/*.c
        ${CMAKE_CURRENT_SOURCE_DIR}/lib/json/*.c
        )

#Test config
file(GLOB SOURCE_FILES_TEST_CONFIG
        ${CMAKE_CURRENT_SOURCE_DIR}/proxy_config/*.c
        ${CMAKE_CURRENT_SOURCE_DIR}/proxy_config/proxy_config_cli/*.c
        ${CMAKE_CURRENT_SOURCE_DIR}/lib/json/*.c
        ${CMAKE_CURRENT_SOURCE_DIR}/proxy_utilities/*.c
        ${CMAKE_CURRENT_SOURCE_DIR}/lib/eui64/eui64.c
        )

#Test eui
file(GLOB SOURCE_FILES_TEST_EUI
        ${CMAKE_CURRENT_SOURCE_DIR}/lib/json/*.c
        ${CMAKE_CURRENT_SOURCE_DIR}/proxy_utilities/*.c
        ${CMAKE_CURRENT_SOURCE_DIR}/proxy_config/pc_settings.c
        ${CMAKE_CURRENT_SOURCE_DIR}/lib/eui64/*.c
        )


add_executable(Presto ${SOURCE_FILES})
add_executable(comm_client ${SOURCE_FILES_TEST_CLIENT})
add_executable(pc_test ${SOURCE_FILES_TEST_CONFIG})
add_executable(eui_test ${SOURCE_FILES_TEST_EUI})


target_link_libraries(Presto ${REQUIRED_LIBRARIES})
target_link_libraries(comm_client ${REQUIRED_LIBRARIES})
target_link_libraries(pc_test ${REQUIRED_LIBRARIES})
target_link_libraries(eui_test ${REQUIRED_LIBRARIES})