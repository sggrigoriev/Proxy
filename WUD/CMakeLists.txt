cmake_minimum_required(VERSION 3.5)

project(WUD C)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/../build)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall") # -std=c9x -lc -Wall -Werror")

set(REQUIRED_LIBRARIES
        m
        pthread
        curl
   )

#all include paths
include_directories(
        wud_config
        wud_fw_upgrade
        wud_monitor
        wud_startup
        wud_watchdog
        wud_actions
        wud_utils
        wud_threads
        ../lib/libtcp
        ../lib/presto_commands
        ../lib/pu_logger
        ../lib/pc_config
        ../lib/pu_queues
        ../lib/libhttpcomm
        ../lib/json
)

#WUD
file(GLOB SOURCE_FILES
        main.c
        wud_config/wc_settings.c
        wud_fw_upgrade/*.c
        wud_monitor/*.c
        wud_startup/*.c
        wud_watchdog/*.c
        wud_actions/*.c
        wud_utils/*.c
        wud_threads/*.c
        ../lib/libtcp/*.c
        ../lib/presto_commands/*.c
        ../lib/pu_logger/*.c
        ../lib/pc_config/pc_config.c
        ../lib/pu_queues/*.c
        ../lib/libhttpcomm/*.c
        ../lib/json/*.c
        )


add_executable(WUD ${SOURCE_FILES})

target_link_libraries(WUD ${REQUIRED_LIBRARIES})
