#cmake_minimum_required(VERSION 3.5)

project(WUD C)
#set (CMAKE_C_STANDARD 90)

if (PRESTO_PLATFORM STREQUAL "cross")

	# MLevitin additions
	set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ../${PRESTO_PLATFORM})
	message(STATUS "Cross-compiling for the MIPS")
	configure_file(wud.conf.mips ../${PRESTO_PLATFORM}/wud.conf COPYONLY)
	set(OPENWRT_PATH $ENV{OPENWRT_PATH})
	set(ROOTFS_PATH    ${OPENWRT_PATH}/staging_dir/target-mipsel_24kec+dsp_uClibc-0.9.33.2)
	set(TOOLCHAIN_PATH ${OPENWRT_PATH}/staging_dir/toolchain-mipsel_24kec+dsp_gcc-4.8-linaro_uClibc-0.9.33.2)
	set(COMPILER_PREFIX mipsel-openwrt-linux-uclibc-)
	set(CMAKE_C_COMPILER ${TOOLCHAIN_PATH}/bin/${COMPILER_PREFIX}gcc)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}   -std=c89  -I${ROOTFS_PATH}/usr/include -L${ROOTFS_PATH}/usr/lib -L${ROOTFS_PATH}/root-ramips/usr/lib  -D_GNU_SOURCE -Wall -s")
	set(REQUIRED_LIBRARIES
			m
			pthread
			curl
			mbedtls
			)
elseif(PRESTO_PLATFORM STREQUAL "ubuntu")
	#file(MAKE_DIRECTORY ../${PRESTO_PLATFORM}/make)
	#set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ../${PRESTO_PLATFORM})
	# To separate run on real HW and on server
	#configure_file(wud.conf.x86 ../${PRESTO_PLATFORM}/wud.conf COPYONLY)
	add_definitions(-DWUD_ON_HOST)
	message(STATUS "Compiling for the X86")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${GCC_STANDARD} -O0 -g -Wall") #-lc
	#	file(MAKE_DIRECTORY build/$ENV{PLATFORM})
	#	set(CMAKE_RUNTIME_OUTPUT_DIRECTORY $ENV{PLATFORM})
	set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/../build)
	set(REQUIRED_LIBRARIES
			m
			pthread
			curl
			)
elseif(PRESTO_PLATFORM STREQUAL "openwrt")
	configure_file(wud.conf.x86 ../${PRESTO_PLATFORM}/wud.conf COPYONLY)
	message(STATUS "Compiling for the OpenWRT")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -O2 -std=c89  -pthread -s")
	set(REQUIRED_LIBRARIES
			m
#			pthread
			curl
			c
			)
elseif (PRESTO_PLATFORM STREQUAL "quark")

	# MLevitin additions
	set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ../${PRESTO_PLATFORM})
	message(STATUS "Cross-compiling for the Quark")
	configure_file(wud.conf.mips ../${PRESTO_PLATFORM}/wud.conf COPYONLY)
	set(QUARK_PATH $ENV{QUARK_PATH})
	set(ROOTFS_PATH    ${QUARK_PATH}/sysroots)
	set(TOOLCHAIN_PATH ${QUARK_PATH}/sysroots/x86_64-linux/usr/bin/i586-ostro-linux)
	set(COMPILER_PREFIX i586-ostro-linux-)
	set(CMAKE_C_COMPILER ${TOOLCHAIN_PATH}/${COMPILER_PREFIX}gcc)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}  -m32 -march=i586  -std=c89  --sysroot=${QUARK_PATH}/sysroots/x86_64-linux  -L${ROOTFS_PATH}/usr/lib -L${ROOTFS_PATH}/root-ramips/usr/lib  -D_GNU_SOURCE -Wall -s")
	set(REQUIRED_LIBRARIES
			m
			pthread
			curl
			)
else()
	message(STATUS "Please set PLATFORM to [cross|ubuntu|openwrt|quark].")
	return()
endif()



#all include paths
include_directories(
		wud_http
		wud_config
		wud_fw_upgrade
		wud_monitor
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
		../lib/libtimer
		../lib/libsha
)

#WUD
file(GLOB SOURCE_FILES
		main.c
		wud_http/*.c
		wud_config/*.c
		wud_fw_upgrade/*.c
		wud_monitor/*.c
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
		../lib/libtimer/*.c
		../lib/libsha/lib_sha_256.c
		)
#SHA256 test
#file(GLOB SHA_SOURCE_FILES
#		../lib/libsha/*.c
#		)

add_executable(WUD ${SOURCE_FILES})
#add_executable(sha256_test ${SHA_SOURCE_FILES})

target_link_libraries(WUD ${REQUIRED_LIBRARIES})
#target_link_libraries(sha256_test ${REQUIRED_LIBRARIES})

if (PRESTO_PLATFORM STREQUAL "openwrt")
INSTALL(TARGETS WUD DESTINATION /build )
INSTALL(FILES wud.conf.mips DESTINATION /build)
endif()

