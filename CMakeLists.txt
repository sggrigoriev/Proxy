cmake_minimum_required(VERSION 2.8.0)
project(Presto C)

set(CMAKE_VERBOSE_MAKEFILE OFF)

    message(STATUS "Target platform is set to '$ENV{PLATFORM}'")

set(PRESTO_PLATFORM $ENV{PLATFORM})
add_subdirectory(Proxy)
add_subdirectory(WUD)
set(GCC_STANDARD "-std=c99")
set(BUILD_TYPE $ENV{BUILD_TYPE})
message (" Build type is ${BUILD_TYPE}")


if (PRESTO_PLATFORM STREQUAL "cross")
    # this is for cross-compile using OpenWRT toolchain
    set(PRESTO_PLATFORM $ENV{PLATFORM})
    message(STATUS "Cross-compile build. Platform is set to ${PRESTO_PLATFORM}")
elseif (PRESTO_PLATFORM STREQUAL "ubuntu")
    # this is for x86/Ubuntu build
    set(PRESTO_PLATFORM $ENV{PLATFORM})
    message(STATUS "Ubuntu/X86 build. Platform is set to ${PRESTO_PLATFORM}")
    set (CMAKE_C_STANDARD 90)
elseif(PRESTO_PLATFORM STREQUAL "openwrt")
    # this is for the OpenWRT target build.
    set(CMAKE_VERBOSE_MAKEFILE OFF)
    set(PRESTO_PLATFORM $ENV{PLATFORM})
    message (STATUS "OpenWRT build. Platform is set to ${PRESTO_PLATFORM}")
    set(GCC_STANDARD "-std=c89")
elseif(PRESTO_PLATFORM STREQUAL "quark")
    # this is for the Quark
    set(CMAKE_VERBOSE_MAKEFILE OFF)
    set(PRESTO_PLATFORM $ENV{PLATFORM})
    message (STATUS "Intel Quark build. Platform is set to ${PRESTO_PLATFORM}")
    set(GCC_STANDARD "-std=c89")
elseif(PRESTO_PLATFORM STREQUAL "hisilicon")
    # this is for Hisilicon
    set(CMAKE_VERBOSE_MAKEFILE OFF)
    set(PRESTO_PLATFORM $ENV{PLATFORM})
    message (STATUS "Hisilicon build. Platform is set to ${PRESTO_PLATFORM}")
else()
    message (STATUS "PLATFORM is undefined. Please set to [ubuntu|cross|openwrt|quark] ${PRESTO_PLATFORM}")
endif()

#NB! do not forget put it into top-level include file!
#if STDC_VERSION >= 199901L
#define _XOPEN_SOURCE 600
#else
#define _XOPEN_SOURCE 500
#endif / STDC_VERSION /