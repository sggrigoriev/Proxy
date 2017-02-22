cmake_minimum_required(VERSION 3.5)

project(FWinstall C)
set (CMAKE_C_STANDARD 90)
message(STATUS "platform is $ENV{PLATFORM}")

#Eneble CURL debug option for POST operations
#add_definitions( -DLIBHTTP_CURL_DEBUG)


	file(MAKE_DIRECTORY build/$ENV{PLATFORM})
	set(CMAKE_RUNTIME_OUTPUT_DIRECTORY $ENV{PLATFORM})

set(GCC_STANDARD $ENV{STANDARD})

if ($ENV{PLATFORM} STREQUAL "cross")
	# MLevitin additions
	message(STATUS "Compiling for the MIPS")
	set(OPENWRT_PATH $ENV{OPENWRT_PATH})
	set(ROOTFS_PATH    ${OPENWRT_PATH}/staging_dir/target-mipsel_24kec+dsp_uClibc-0.9.33.2)
	set(TOOLCHAIN_PATH ${OPENWRT_PATH}/staging_dir/toolchain-mipsel_24kec+dsp_gcc-4.8-linaro_uClibc-0.9.33.2)
	set(COMPILER_PREFIX mipsel-openwrt-linux-uclibc-)
	set(CMAKE_C_COMPILER ${TOOLCHAIN_PATH}/bin/${COMPILER_PREFIX}gcc)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${GCC_STANDARD} -I${ROOTFS_PATH}/usr/include -L${ROOTFS_PATH}/usr/lib -L${ROOTFS_PATH}/root-ramips/usr/lib -D_GNU_SOURCE -O3 -Wall -s")
	set(REQUIRED_LIBRARIES
			m
			pthread
			curl
			mbedtls
		)
elseif($ENV{PLATFORM} STREQUAL "host")
  	message(STATUS "Compiling for the X86")
#    file(MAKE_DIRECTORY build/$ENV{PLATFORM})
#    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY $ENV{PLATFORM})
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}  ${GCC_STANDARD} -D_GNU_SOURCE -std=c90 -Wall")
#	set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/../build)
	set(REQUIRED_LIBRARIES
			m
			pthread
			curl
		)
else()
	message(STATUS "Please select PLATFORM host or cross")
	return()
endif()

#all include paths
include_directories(
		../Proxy
        ../lib/pu_logger
        ../lib/libhttpcomm
		../lib/json
)

#FWinstall
file(GLOB SOURCE_FILES
        main.c
        ../lib/pu_logger/*.c
        ../lib/libhttpcomm/*.c
		../lib/json/*.c
        )

add_executable(FWinstall ${SOURCE_FILES})

target_link_libraries(FWinstall ${REQUIRED_LIBRARIES})
