#cmake_minimum_required(VERSION 3.5)

project(Proxy C)
#set (CMAKE_C_STANDARD 90)
message(STATUS "Proxy: platform is set to ${PRESTO_PLATFORM}")

#ifdef PROXY_SEPARATE_RUN - the proxy can read parameters from command line first
#else - the proxy reads all configuration from default configuration file only

#add_definitions( -DPROXY_SEPARATE_RUN )
#Eneble CURL debug option for POST operations
#add_definitions( -DLIBHTTP_CURL_DEBUG)

#Shouild be switched off! Just for internel use
#add_definitions( -DPROXY_AUTH_GET_EMUL)
#set(CMAKE_C_COMPILER /home/gsg/openwrt/staging_dir/toolchain-mipsel_24kec+dsp_gcc-4.8-linaro_uClibc-0.9.33.2/bin/mipsel-openwrt-linux-uclibc-gcc)

#staging_dir/target-mipsel_24kec+dsp_uClibc-0.9.33.2/root-ramips/usr/lib/libmbedtls.a
#build_dir/target-mipsel_24kec+dsp_uClibc-0.9.33.2/root-ramips/usr/lib/libmbedtls.a


set(GCC_STANDARD $ENV{STANDARD})

if (PRESTO_PLATFORM STREQUAL "cross")
	# MLevitin additions
	file(MAKE_DIRECTORY ../${PRESTO_PLATFORM})
	set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ../${PRESTO_PLATFORM})
	message(STATUS "Compiling for the MIPS")
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
  	message(STATUS "Compiling for the X86")
	#configure_file(proxyJSON.conf.x86 ../${PRESTO_PLATFORM}/proxyJSON.conf COPYONLY)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${GCC_STANDARD} -O0 -Wall") #${GCC_STANDARD} -D_GNU_SOURCE
	#	file(MAKE_DIRECTORY build/$ENV{PLATFORM})
	#	set(CMAKE_RUNTIME_OUTPUT_DIRECTORY $ENV{PLATFORM})
	#	set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/../build)
	set(REQUIRED_LIBRARIES
			m
			pthread
			curl
		)
elseif(PRESTO_PLATFORM STREQUAL "openwrt")
  	message(STATUS "Compiling for the OpenWRT")
	configure_file(proxyJSON.conf.x86 ../${PRESTO_PLATFORM}/proxyJSON.conf COPYONLY)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D_GNU_SOURCE -Wall -O2 -std=c89 -pthread -s")
	set(REQUIRED_LIBRARIES
			m
#			pthread
			curl
#			mbedtls
			c
		)
else()
	message(STATUS "Please set PLATFORM to[cross|ubuntu|openwrt]")
	return()
endif()


#all include paths
include_directories(
		./
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
#        proxy_threads/*.c
        proxy_config/pc_settings.c
        proxy_eui64/eui64.c
#		proxy_http/*.c
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
    target_link_libraries(Proxy ${REQUIRED_LIBRARIES})
    INSTALL(TARGETS Proxy DESTINATION /build )
    INSTALL(FILES proxyJSON.conf.mips DESTINATION /build )
else()
    add_executable(Proxy ${SOURCE_FILES})
    add_executable(comm_client ${SOURCE_FILES_TEST_CLIENT})

    target_link_libraries(Proxy ${REQUIRED_LIBRARIES})
    target_link_libraries(comm_client ${REQUIRED_LIBRARIES})
endif()

