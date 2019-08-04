#cmake_minimum_required(VERSION 3.5)

project(WUD C)
#set (CMAKE_C_STANDARD 90)

add_definitions(-DDEFAUT_U_TO=200)
add_definitions(-DDEFAULT_TO_RPT_TIMES=5)
add_definitions(-DDEFAULT_S_TO=1)

message(STATUS "WUD: platform is set to ${PRESTO_PLATFORM}")
set(BUILD_TYPE $ENV{BUILD_TYPE})
message (" Proxy Build type is ${BUILD_TYPE}")

execute_process(
		COMMAND  bash "-c" "git rev-parse --verify HEAD | tr -d '\n'"
		RESULT_VARIABLE result
		OUTPUT_VARIABLE commit_hash)

execute_process(
		COMMAND bash "-c" "git rev-parse --abbrev-ref HEAD | tr -d '\n'"
		RESULT_VARIABLE result
		OUTPUT_VARIABLE branch)

execute_process(
		COMMAND bash "-c" "date '+%Y-%m-%d.%H:%M:%S' | tr -d '\n'"
		RESULT_VARIABLE result
		OUTPUT_VARIABLE build_date)

execute_process(
		COMMAND bash "-c" "git diff --exit-code"
		RESULT_VARIABLE git_diff
		OUTPUT_VARIABLE git_diff_output)

execute_process(
		COMMAND bash "-c" "git config --get remote.origin.url | tr -d '\n'"
		RESULT_VARIABLE result
		OUTPUT_VARIABLE git_url)


if (git_diff STREQUAL "0")
	message("No changes '${git_diff}'")
else()
	message("!!!! Uncommited changes '${git_diff}' !!!!!!!!")
endif()


message("value is  'Commit:${commit_hash} Branch:${branch}: Date:${build_date}' URL: ${git_url}")


add_definitions(-DGIT_COMMIT="${commit_hash}")
add_definitions(-DGIT_BRANCH="${branch}")
add_definitions(-DGIT_URL="${git_url}")
add_definitions(-DBUILD_DATE="${build_date}")
add_definitions(-DUNCOMMITED_CHANGES=${git_diff})


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
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}   -std=c89  -I${ROOTFS_PATH}/usr/include -L${ROOTFS_PATH}/usr/lib -L${ROOTFS_PATH}/root-ramips/usr/lib -Wall -s")
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
	#add_definitions(-DWUD_NOT_STARTS_AGENT)											# separate Agent run if not commented
	#add_definitions(-DWUD_ON_HOST)														# enable if UBUNTU run!
	message(STATUS "Compiling for the X86.")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${GCC_STANDARD} -O0 -g -Wall") #-lc
	#	file(MAKE_DIRECTORY build/$ENV{PLATFORM})
	set(CMAKE_RUNTIME_OUTPUT_DIRECTORY $ENV{PRESTO_PLATFORM})
	set(REQUIRED_LIBRARIES
			m
			pthread
			curl
			)
elseif(PRESTO_PLATFORM STREQUAL "openwrt")
	add_definitions(-D WUD_NOT_STARTS_AGENT)
	configure_file(wud.conf.x86 ../${PRESTO_PLATFORM}/wud.conf COPYONLY)
	message(STATUS "Compiling for the OpenWRT")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -O2 -std=c89 -D_BSD_SOURCE -pthread -s")
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
elseif (PRESTO_PLATFORM STREQUAL "hisilicon")

#	file(MAKE_DIRECTORY ../${PRESTO_PLATFORM})
	set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ../${PRESTO_PLATFORM})

	set(CMAKE_SYSTEM_NAME Linux)

	add_definitions(-DPROXY_ETHERNET_INTERFACE="eth0")
	add_definitions(-DPLATFORM_HISILICON_C1)


	set(TOOLCHAIN_PATH $ENV{TOOLCHAIN})
	set(SYSROOT_PATH $ENV{SYSROOT})
	set(PP_SDK_PATH $ENV{SDK_PATH})
	set(PROXY_LIBS_PATH ${PP_SDK_PATH}/lib)
	set(COMPILER_PREFIX arm-hisiv500-linux-uclibcgnueabi-)
	set(CMAKE_C_COMPILER ${TOOLCHAIN_PATH}/bin/${COMPILER_PREFIX}gcc)
#	set(CMAKE_SYSROOT ${TOOLCHAIN_PATH}/target/usr)
	set(CMAKE_FORCE_C_COMPILER ${TOOLCHAIN_PATH}/bin/${COMPILER_PREFIX}gcc GNU)
	set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
	set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
	set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
	set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)


#	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -I${SYSROOT_PATH}/usr/include/gstreamer-1.0 -I${SYSROOT_PATH}/usr/include/glib-2.0 -I${SYSROOT_PATH}/usr/include -D_BSD_SOURCE -L${SYSROOT_PATH}/usr/lib -D_GNU_SOURCE -O3 -Wall -s")
#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}  -I${SYSROOT_PATH}/usr/include -L${SYSROOT_PATH}/usr/lib -L${TOOLCHAIN_PATH}/target/lib -D_GNU_SOURCE -Wall -O2 -std=c89 -D_BSD_SOURCE -pthread -s")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}  -I${SYSROOT_PATH}/usr/include -L${SYSROOT_PATH}/usr/lib -L${TOOLCHAIN_PATH}/target/lib -D_GNU_SOURCE -D_BSD_SOURCE -Wall -std=c89")
	if(BUILD_TYPE STREQUAL "Development")
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O0")
	else()
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}  -O3 -s ")
	endif()
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pthread")



set(REQUIRED_LIBRARIES
			curl
			m
			pthread
			nopoll
			ssl
			crypto
			dl)
	message(STATUS "Compiling for hiSilicon CPU (${CMAKE_SYSROOT})")
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
elseif (PRESTO_PLATFORM STREQUAL "hisilicon")
	target_link_libraries(WUD ${REQUIRED_LIBRARIES}

			${SYSROOT_PATH}/usr/lib/libz.a
			${SYSROOT_PATH}/usr/lib/libgettextlib.a

			${SYSROOT_PATH}/usr/lib/libintl.a
			${SYSROOT_PATH}/usr/lib/libffi.a
			${SYSROOT_PATH}/usr/lib/libcurl.a
#			${SYSROOT_PATH}/usr/lib/libdl.so.0

			)
	INSTALL(TARGETS WUD DESTINATION /build )
	INSTALL(FILES wud.conf.mips DESTINATION /build )
endif()

