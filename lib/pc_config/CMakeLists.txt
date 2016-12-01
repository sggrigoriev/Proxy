cmake_minimum_required(VERSION 3.6)

project(pc_config_test C)

set (CMAKE_RUNTIME_OUTPUT_DIRECTORY /media/sf_GWDrive/Presto_new/build)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall") # -std=c9x -lc -Wall -Werror")

set(REQUIRED_LIBRARIES
        m
        pthread
        curl
        )

#all include paths
include_directories(
        ./
        ../json
)

#WUD
file(GLOB SOURCE_FILES
        ./*.c
        ../json/*.c
        )


add_executable(pc_config_test ${SOURCE_FILES})

target_link_libraries(pc_config_test ${REQUIRED_LIBRARIES})
