cmake_minimum_required(VERSION 3.2.2)
project(Presto C)

add_definitions( -DGIT_FIRMWARE_VERSION=301 )

add_subdirectory(Proxy)
add_subdirectory(WUD)