cmake_minimum_required(VERSION 2.8.0)
project(Presto C)

#set(CMAKE_VERBOSE_MAKEFILE ON)
set(PRESTO_PLATFORM $ENV{PLATFORM})

if (PRESTO_PLATFORM STREQUAL "cross")
    set(PRESTO_PLATFORM $ENV{PLATFORM})
    message(STATUS "platform is set to ${PRESTO_PLATFORM}") 
    set(GCC_STANDARD "-std=c99")
    add_subdirectory(Proxy)
    add_subdirectory(WUD)
elseif (PRESTO_PLATFORM STREQUAL "host")
    set(PRESTO_PLATFORM $ENV{PLATFORM})
    message(STATUS "platform is set to ${PRESTO_PLATFORM}") 
    set (CMAKE_C_STANDARD 90)
    set(GCC_STANDARD "-std=c99")
    add_subdirectory(Proxy)
    add_subdirectory(WUD)
    add_subdirectory(FWinstall)
else()
    set(CMAKE_VERBOSE_MAKEFILE ON)
    message (STATUS "Assuming it is OpenWRT build (PLATFORM='$ENV{PLATFORM}')")
    set(PRESTO_PLATFORM "OpenWRT")
    set(GCC_STANDARD "-std=c99")
    add_subdirectory(Proxy)
    add_subdirectory(WUD)
endif()

#NB! do not forget put it into top-level include file!
#if STDC_VERSION >= 199901L
#define _XOPEN_SOURCE 600
#else
#define _XOPEN_SOURCE 500
#endif / STDC_VERSION /