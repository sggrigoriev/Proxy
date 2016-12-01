cmake_minimum_required(VERSION 3.2.2)
project(Presto C)


set (CMAKE_RUNTIME_OUTPUT_DIRECTORY /media/sf_GWDrive/Presto_new/build)

message(${CMAKE_RUNTIME_OUTPUT_DIRECTORY})

add_definitions( -DGIT_FIRMWARE_VERSION=301 )

add_subdirectory(Proxy)
add_subdirectory(WUD)
add_subdirectory(lib/pc_config)