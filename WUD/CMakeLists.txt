cmake_minimum_required(VERSION 3.5)

project(WUD C)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/../build)



if ($ENV{PLATFORM} STREQUAL "cross")
	# MLevitin additions
	message(STATUS "Compiling for the MIPS")
	file(MAKE_DIRECTORY build/$ENV{PLATFORM})
	set(CMAKE_RUNTIME_OUTPUT_DIRECTORY $ENV{PLATFORM})
	set(OPENWRT_PATH $ENV{OPENWRT_PATH})
	set(ROOTFS_PATH    ${OPENWRT_PATH}/staging_dir/target-mipsel_24kec+dsp_uClibc-0.9.33.2)
	set(TOOLCHAIN_PATH ${OPENWRT_PATH}/staging_dir/toolchain-mipsel_24kec+dsp_gcc-4.8-linaro_uClibc-0.9.33.2)
	set(COMPILER_PREFIX mipsel-openwrt-linux-uclibc-)
	set(CMAKE_C_COMPILER ${TOOLCHAIN_PATH}/bin/${COMPILER_PREFIX}gcc)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}  -std=c9x -I${ROOTFS_PATH}/usr/include -L${ROOTFS_PATH}/usr/lib -L${ROOTFS_PATH}/root-ramips/usr/lib  -D_GNU_SOURCE -Wall -s")
	set(REQUIRED_LIBRARIES
	    m
	    pthread
	    curl
	    mbedtls
	    )
elseif($ENV{PLATFORM} STREQUAL "host")
	    message(STATUS "Compiling for the X86")
	    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall") # -std=c9x -lc -Wall -Werror")
	    file(MAKE_DIRECTORY build/$ENV{PLATFORM})
	    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY $ENV{PLATFORM})
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
