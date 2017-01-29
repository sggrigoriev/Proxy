cmake_minimum_required(VERSION 3.5)

project(Proxy C)
message(STATUS "platform is $ENV{PLATFORM}")

add_definitions( -DGIT_FIRMWARE_VERSION=301 )
#ifdef PROXY_SEPARATE_RUN - the proxy can read parameters from command line first
#else - the proxy reads all configuration from default configuration file only
add_definitions( -DPROXY_SEPARATE_RUN )
#Eneble CURL debug option for POST operations
#add_definitions( -DLIBHTTP_CURL_DEBUG)

#set(CMAKE_C_COMPILER /home/gsg/openwrt/staging_dir/toolchain-mipsel_24kec+dsp_gcc-4.8-linaro_uClibc-0.9.33.2/bin/mipsel-openwrt-linux-uclibc-gcc)

	file(MAKE_DIRECTORY build/$ENV{PLATFORM})
	set(CMAKE_RUNTIME_OUTPUT_DIRECTORY $ENV{PLATFORM})
	configure_file(proxyJSON.conf $ENV{PLATFORM}/proxyJSON.conf COPYONLY)


#staging_dir/target-mipsel_24kec+dsp_uClibc-0.9.33.2/root-ramips/usr/lib/libmbedtls.a
#build_dir/target-mipsel_24kec+dsp_uClibc-0.9.33.2/root-ramips/usr/lib/libmbedtls.a



if ($ENV{PLATFORM} STREQUAL "cross")
	# MLevitin additions
	message(STATUS "Compiling for the MIPS")
	set(OPENWRT_PATH $ENV{OPENWRT_PATH})
	set(ROOTFS_PATH    ${OPENWRT_PATH}/staging_dir/target-mipsel_24kec+dsp_uClibc-0.9.33.2)
	set(TOOLCHAIN_PATH ${OPENWRT_PATH}/staging_dir/toolchain-mipsel_24kec+dsp_gcc-4.8-linaro_uClibc-0.9.33.2)
	set(COMPILER_PREFIX mipsel-openwrt-linux-uclibc-)
	set(CMAKE_C_COMPILER ${TOOLCHAIN_PATH}/bin/${COMPILER_PREFIX}gcc)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}  -std=c9x -I${ROOTFS_PATH}/usr/include -L${ROOTFS_PATH}/usr/lib -L${ROOTFS_PATH}/root-ramips/usr/lib -D_GNU_SOURCE -Wall -s")
	set(REQUIRED_LIBRARIES
			m
			pthread
			curl
			mbedtls
		)
elseif($ENV{PLATFORM} STREQUAL "host")
	message(STATUS "Compiling for the X86")
    file(MAKE_DIRECTORY build/$ENV{PLATFORM})
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY $ENV{PLATFORM})
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
        proxy_functions
        proxy_threads
        proxy_config
        proxy_config/proxy_config_cli
        proxy_eui64
		proxy_http
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
        proxy_eui64/eui64.c
		proxy_http/*.c
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
        proxy_functions/*.c
        proxy_threads/*.c
        proxy_config/pc_settings.c
        proxy_eui64/eui64.c
		proxy_http/*.c
        ../lib/presto_commands/*.c
        ../lib/libtcp/*.c
        ../lib/pu_logger/*.c
        ../lib/pc_config/pc_config.c
        ../lib/pu_queues/*.c
        ../lib/libhttpcomm/*.c
		../lib/libtimer/*.c
        ../lib/json/*.c
        )
#Test server
#file(GLOB SOURCE_FILES_TEST_SERVER
#        test_server/comm_server.c
#        proxy_threads/*.c
#        proxy_config/pc_settings.c
#        ../lib/pt_tcp_utl/*.c
#        ../lib/pu_logger/*.c
#        ../lib/pc_config/pc_config.c
#        ../lib/pu_queues/*.c
#        ../lib/libhttpcomm/*.c
#        ../lib/json/*.c
#        )

#Test config
#file(GLOB SOURCE_FILES_TEST_CONFIG
#        proxy_config/*.c
#        proxy_config/proxy_config_cli/*.c
#        proxy_eui64/eui64.c
#        ../lib/pt_tcp_utl/*.c
#        ../lib/pu_logger/*.c
#        ../lib/pc_config/pc_config.c
#        ../lib/pu_queues/*.c
#        ../lib/json/*.c
#        )

#Test eui
#file(GLOB SOURCE_FILES_TEST_EUI
#        ../lib/json/*.c
#        proxy_config/pc_settings.c
#        proxy_eui64/*.c
#        ../lib/pu_queues/*.c
#        ../lib/pt_tcp_utl/*.c
#        ../lib/pu_logger/*.c
#        ../lib/pc_config/pc_config.c
#        )


add_executable(Proxy ${SOURCE_FILES})
add_executable(comm_client ${SOURCE_FILES_TEST_CLIENT} proxy_threads/pt_wud_write.c proxy_threads/pt_wud_write.h)
#add_executable(comm_server ${SOURCE_FILES_TEST_SERVER})
#add_executable(pc_test ${SOURCE_FILES_TEST_CONFIG})
#add_executable(eui_test ${SOURCE_FILES_TEST_EUI})


target_link_libraries(Proxy ${REQUIRED_LIBRARIES})
target_link_libraries(comm_client ${REQUIRED_LIBRARIES})
#target_link_libraries(comm_server ${REQUIRED_LIBRARIES})
#target_link_libraries(pc_test ${REQUIRED_LIBRARIES})
#target_link_libraries(eui_test ${REQUIRED_LIBRARIES})