#cmake_minimum_required(VERSION 3.5)

project(Proxy C)
#set (CMAKE_C_STANDARD 90)
message(STATUS "Proxy: platform is set to ${PRESTO_PLATFORM}")
set(BUILD_TYPE $ENV{BUILD_TYPE})
message (" Proxy Build type is ${BUILD_TYPE}")

#Eneble CURL debug option for POST operations
#add_definitions( -DLIBHTTP_CURL_DEBUG)

add_definitions(-DDEFAUT_U_TO=200)
add_definitions(-DDEFAULT_TO_RPT_TIMES=5)
add_definitions(-DDEFAULT_S_TO=1)

#Shouild be switched off! Just for internal use
#add_definitions( -DPROXY_AUTH_GET_EMUL)
#set(CMAKE_C_COMPILER /home/gsg/openwrt/staging_dir/toolchain-mipsel_24kec+dsp_gcc-4.8-linaro_uClibc-0.9.33.2/bin/mipsel-openwrt-linux-uclibc-gcc)

#staging_dir/target-mipsel_24kec+dsp_uClibc-0.9.33.2/root-ramips/usr/lib/libmbedtls.a
#build_dir/target-mipsel_24kec+dsp_uClibc-0.9.33.2/root-ramips/usr/lib/libmbedtls.a


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

add_definitions(-DGIT_COMMIT="${commit_hash}")
add_definitions(-DGIT_BRANCH="${branch}")
add_definitions(-DGIT_URL="${git_url}")
add_definitions(-DBUILD_DATE="${build_date}")
add_definitions(-DUNCOMMITED_CHANGES=${git_diff})


message("value is  'Commit:${commit_hash} Branch:${branch}: Date:${build_date}' URL: ${git_url}")

set(GCC_STANDARD $ENV{STANDARD})

if (PRESTO_PLATFORM STREQUAL "cross")
	# MLevitin additions
	file(MAKE_DIRECTORY ../${PRESTO_PLATFORM})
	set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ../${PRESTO_PLATFORM})
	message(STATUS "Compiling for the MIPS")
	add_definitions(-DPROXY_ETHERNET_INTERFACE="br-lan")
	configure_file(proxyJSON.conf.mips ../${PRESTO_PLATFORM}/proxyJSON.conf COPYONLY)
	set(OPENWRT_PATH $ENV{OPENWRT_PATH})
	set(ROOTFS_PATH    ${OPENWRT_PATH}/staging_dir/target-mipsel_24kec+dsp_uClibc-0.9.33.2)
	set(TOOLCHAIN_PATH ${OPENWRT_PATH}/staging_dir/toolchain-mipsel_24kec+dsp_gcc-4.8-linaro_uClibc-0.9.33.2)
	set(COMPILER_PREFIX mipsel-openwrt-linux-uclibc-)
	set(CMAKE_C_COMPILER ${TOOLCHAIN_PATH}/bin/${COMPILER_PREFIX}gcc)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c89 -I${ROOTFS_PATH}/usr/include -D_BSD_SOURCE -L${ROOTFS_PATH}/usr/lib -L${ROOTFS_PATH}/root-ramips/usr/lib -D_GNU_SOURCE -O3 -Wall -s")
	set(REQUIRED_LIBRARIES
			m
			pthread
			curl
			mbedtls
		)
elseif(PRESTO_PLATFORM STREQUAL "ubuntu")
    # To separate run on real HW and on server
	#file(MAKE_DIRECTORY ../${PRESTO_PLATFORM})
	#set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ../${PRESTO_PLATFORM})
	add_definitions(-DPROXY_ON_HOST)
	add_definitions(-DPROXY_ETHERNET_INTERFACE="eth0")
  	message(STATUS "Compiling for the X86")
	#configure_file(proxyJSON.conf.x86 ../${PRESTO_PLATFORM}/proxyJSON.conf COPYONLY)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${GCC_STANDARD} -O0 -g -Wall") #${GCC_STANDARD} -D_GNU_SOURCE
	#	file(MAKE_DIRECTORY build/$ENV{PLATFORM})
	set(CMAKE_RUNTIME_OUTPUT_DIRECTORY $ENV{PRESTO_PLATFORM})
	set(REQUIRED_LIBRARIES
			m
			pthread
			curl
		)
elseif(PRESTO_PLATFORM STREQUAL "openwrt")
  	message(STATUS "Compiling for the OpenWRT")
	add_definitions(-DPROXY_ETHERNET_INTERFACE="br-lan")
	configure_file(proxyJSON.conf.x86 ../${PRESTO_PLATFORM}/proxyJSON.conf COPYONLY)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D_GNU_SOURCE -Wall -O2 -std=c89 -D_BSD_SOURCE -pthread -s")
	set(REQUIRED_LIBRARIES
			m
#			pthread
#			curl
			mbedtls
#			$ENV{STAGING_DIR}/target-mipsel_24kec+dsp_uClibc-0.9.33.2/usr/lib/libcurl.a
			$ENV{STAGING_DIR}/usr/lib/libcurl.a
			$ENV{STAGING_DIR}/usr/lib/libmbedtls.a
			c
		)
elseif (PRESTO_PLATFORM STREQUAL "quark")
		# MLevitin additions
		file(MAKE_DIRECTORY ../${PRESTO_PLATFORM})
		set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ../${PRESTO_PLATFORM})
		message(STATUS "Compiling Proxy  for the Quark")
		add_definitions(-DPROXY_ETHERNET_INTERFACE="enp0s20f6")
		configure_file(proxyJSON.conf.mips ../${PRESTO_PLATFORM}/proxyJSON.conf COPYONLY)
		set(QUARK_PATH $ENV{QUARK_PATH})
		set(ROOTFS_PATH    ${QUARK_PATH}/sysroots)
		set(TOOLCHAIN_PATH ${QUARK_PATH}/sysroots/x86_64-linux/usr/bin/i586-ostro-linux)
		set(COMPILER_PREFIX i586-ostro-linux-)
		set(CMAKE_C_COMPILER ${TOOLCHAIN_PATH}/${COMPILER_PREFIX}gcc)
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m32 -march=i586 -std=c89 --sysroot=${QUARK_PATH}/sysroots/x86_64-linux -D_DEFAULT_SOURCE -L${ROOTFS_PATH}/usr/lib -L${ROOTFS_PATH}/root-ramips/usr/lib -D_GNU_SOURCE -O3 -Wall -s")
		set(REQUIRED_LIBRARIES
				m
				pthread
				curl
				)

elseif (PRESTO_PLATFORM STREQUAL "hisilicon")

	add_definitions(-DPROXY_ETHERNET_INTERFACE="ra0")
	add_definitions(-D__LINUX__)
	add_definitions(-DEXTERN=)
	set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ../${PRESTO_PLATFORM})
	set(CMAKE_SYSTEM_NAME Linux)
	#add_definitions( -DLIBHTTP_CURL_DEBUG)

	set(TOOLCHAIN_PATH $ENV{TOOLCHAIN})
	set(SYSROOT_PATH $ENV{SYSROOT})
	set(PP_SDK_PATH $ENV{SDK_PATH})
	set(PROXY_LIBS_PATH ${PP_SDK_PATH}/lib)
	set(COMPILER_PREFIX arm-hisiv500-linux-uclibcgnueabi-)
	set(CMAKE_C_COMPILER ${TOOLCHAIN_PATH}/bin/${COMPILER_PREFIX}gcc)
#	set(CMAKE_SYSROOT ${TOOLCHAIN_PATH}/target/usr)
	set(CMAKE_FORCE_C_COMPILER ${TOOLCHAIN_PATH}/bin/${COMPILER_PREFIX}gcc GNU)

	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}  -I${SYSROOT_PATH}/usr/include -L${SYSROOT_PATH}/usr/lib -L${TOOLCHAIN_PATH}/target/lib -D_GNU_SOURCE -D_BSD_SOURCE -Wall -std=c89")
	if(BUILD_TYPE STREQUAL "Development")
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O0")
	else()
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}  -O3 -s ")
	endif()
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pthread")

	set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
	set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
	set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
	set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)


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
	message(STATUS "Please set PLATFORM to[cross|ubuntu|openwrt|quark]")
	return()
endif()


#all include paths
include_directories(
		./
        proxy_functions
        proxy_threads
        proxy_config
        proxy_config/proxy_config_cli
		proxy_http
        ../lib/proxy_eui64
		../lib/libtimer
        ../lib/presto_commands
        ../lib/libtcp
        ../lib/pu_logger
        ../lib/pc_config
        ../lib/pu_queues
        ../lib/libhttpcomm
        ../lib/json
)

#Presto
file(GLOB SOURCE_FILES
        main.c
        proxy_functions/*.c
        proxy_threads/*.c
        proxy_config/pc_settings.c
        proxy_config/proxy_config_cli/*.c
		proxy_http/*.c
        ../lib/proxy_eui64/eui64.c
		../lib/libtimer/*.c
        ../lib/presto_commands/*.c
        ../lib/libtcp/*.c
        ../lib/pu_logger/*.c
        ../lib/pc_config/pc_config.c
        ../lib/pu_queues/*.c
        ../lib/libhttpcomm/*.c
        ../lib/json/*.c
        )

#Test client
file(GLOB SOURCE_FILES_TEST_CLIENT
        test_client/comm_client.c
		test_client/cc_queues.c
		test_client/cc_emulator.c
        proxy_functions/*.c
#        proxy_threads/*.c
        proxy_config/pc_settings.c
		proxy_http/*.c
		../lib/proxy_eui64/eui64.c
        ../lib/presto_commands/*.c
        ../lib/libtcp/*.c
        ../lib/pu_logger/*.c
        ../lib/pc_config/pc_config.c
        ../lib/pu_queues/*.c
        ../lib/libhttpcomm/*.c
		../lib/libtimer/*.c
        ../lib/json/*.c
        )

if (PRESTO_PLATFORM STREQUAL "openwrt")
    add_executable(Proxy ${SOURCE_FILES})
    target_link_libraries(Proxy ${REQUIRED_LIBRARIES} )
    INSTALL(TARGETS Proxy DESTINATION /build )
    INSTALL(FILES proxyJSON.conf.mips DESTINATION /build )
	INSTALL(FILES cloud_url DESTINATION /build )
elseif (PRESTO_PLATFORM STREQUAL "hisilicon")
	add_executable(Proxy ${SOURCE_FILES})
	target_link_libraries(Proxy ${REQUIRED_LIBRARIES}

			${SYSROOT_PATH}/usr/lib/libz.a
			${SYSROOT_PATH}/usr/lib/libgettextlib.a

			${SYSROOT_PATH}/usr/lib/libintl.a
			${SYSROOT_PATH}/usr/lib/libffi.a
			${SYSROOT_PATH}/usr/lib/libcurl.a
#			${SYSROOT_PATH}/usr/lib/libdl.so.0

			)
	INSTALL(TARGETS Proxy DESTINATION /build )
	INSTALL(FILES proxyJSON.conf.mips DESTINATION /build )
	INSTALL(FILES cloud_url DESTINATION /build )

else()
    add_executable(Proxy ${SOURCE_FILES})
    add_executable(comm_client ${SOURCE_FILES_TEST_CLIENT})

    target_link_libraries(Proxy ${REQUIRED_LIBRARIES})
    target_link_libraries(comm_client ${REQUIRED_LIBRARIES})
endif()

