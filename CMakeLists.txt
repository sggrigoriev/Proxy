cmake_minimum_required(VERSION 3.2.2)
project(Presto C)


set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/build)

message(${CMAKE_RUNTIME_OUTPUT_DIRECTORY})

add_subdirectory(Proxy)
add_subdirectory(WUD)
#add_subdirectory(lib/pc_config)