cmake_minimum_required(VERSION 3.5)
project(Presto C)

#set(CMAKE_VERBOSE_MAKEFILE ON)

add_subdirectory(Proxy)
add_subdirectory(WUD)
add_subdirectory(FWinstall)

#NB! do not forget put it into top-level include file!
#if STDC_VERSION >= 199901L
#define _XOPEN_SOURCE 600
#else
#define _XOPEN_SOURCE 500
#endif / STDC_VERSION /
