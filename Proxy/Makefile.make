# -*- makefile -*-
#
#	makefile for writing configurations into a file
#
# @author mlevitin

include ../support/make/Makefile.include

# What is the main file we want to compile
TARGET = Proxy

# Which file(s) are we trying to test
SOURCES_C = main.c
SOURCES_C += ./proxy_config/proxy_config_cli/pc_cli.c
SOURCES_C += ./proxy_config/pc_settings.c
SOURCES_C += ./proxy_eui64/eui64.c
SOURCES_C += ./proxy_functions/pf_cloud_conn_params.c
SOURCES_C += ./proxy_functions/pf_proxy_commands.c
SOURCES_C += ./proxy_functions/pf_reboot.c
SOURCES_C += ./proxy_http/ph_manager.c
SOURCES_C += ./proxy_threads/pt_agent_read.c
SOURCES_C += ./proxy_threads/pt_agent_write.c
SOURCES_C += ./proxy_threads/pt_main_agent.c
SOURCES_C += ./proxy_threads/pt_queues.c
SOURCES_C += ./proxy_threads/pt_server_read.c
SOURCES_C += ./proxy_threads/pt_server_write.c
SOURCES_C += ./proxy_threads/pt_threads.c
SOURCES_C += ./proxy_threads/pt_wud_write.c
SOURCES_C += ./proxy_threads/pt_change_cloud_notification.c
SOURCES_C += ../lib/libtimer/lib_timer.c
SOURCES_C += ../lib/presto_commands/pf_traffic_proc.c
SOURCES_C += ../lib/presto_commands/pr_commands.c
SOURCES_C += ../lib/presto_commands/pr_ptr_list.c
SOURCES_C += ../lib/libtcp/lib_tcp.c
SOURCES_C += ../lib/pu_logger/pu_logger.c
SOURCES_C += ../lib/pc_config/pc_config.c
SOURCES_C += ../lib/pu_queues/pu_queue.c
SOURCES_C += ../lib/libhttpcomm/lib_http.c
SOURCES_C += ../lib/json/cJSON.c


# Which test(s) are we trying to run
SOURCES_CPP =

# Where are all of our directories we should include
CFLAGS += -I./
CFLAGS += -I./proxy_config
CFLAGS += -I./proxy_config/proxy_config_cli
CFLAGS += -I./proxy_eui64
CFLAGS += -I./proxy_functions
CFLAGS += -I./proxy_http
CFLAGS += -I./proxy_threads
CFLAGS += -I../lib/libtimer
CFLAGS += -I../lib/presto_commands
CFLAGS += -I../lib/libtcp
CFLAGS += -I../lib/pu_logger
CFLAGS += -I../lib/pc_config
CFLAGS += -I../lib/pu_queues
CFLAGS += -I../lib/libhttpcomm
CFLAGS += -I../lib/json

# What 3rd party library headerse should we include.
# Version information is pulled from support/make/Makefile.include
#CFLAGS += -I../../lib/3rdparty/${LIBXML2_VERSION}/include
#CFLAGS += -I../../lib/3rdparty/${LIBCURL_VERSION}/include
#CFLAGS += -I../../lib/3rdparty/${OPENSSL_VERSION}/include
#CFLAGS += -I../../lib/3rdparty/${CJSON_VERSION}

# Version information
CFLAGS += -DGIT_FIRMWARE_VERSION=$(shell git log -1 --pretty=format:0x%h)

#CC = gcc
#CPP = g++
#AR = ar
#STRIP=strip
#INTEL = 0
#export HARDWARE_PLATFORM = INTEL

OBJECTS_C = $(SOURCES_C:.c=.o)
OBJECTS_CPP = $(SOURCES_CPP:.cpp=.o)

LDEXTRA +=  -lcurl -lpthread -lm
ifeq ($(HOST), mips-linux)
    LDEXTRA+=-lmbedtls
    LDFLAGS += -Wl,-rpath $(ROOTFS_PATH)/usr/lib
    CFGNAME=proxyJSON.conf.mips
endif

ifeq ($(HOST), intel-quark)
#    LDEXTRA+=-lmbedtls
    LDFLAGS += -Wl,-rpath $(ROOTFS_PATH)/usr/lib
    CFGNAME=proxyJSON.conf.mips
endif

CFLAGS += -Os
CFLAGS += -Wall

all:	$(TARGET)

.bin:
	@mkdir -p ../bin/$(HOST)/$(TARGET)

.c.o:
	@$(CC) -c $(CFLAGS) -o ../bin/$(HOST)/$(TARGET)/$(shell basename $@) $<

.cpp.o:
	@mkdir ../bin/$(HOST)
	@$(CPP) -c $(CFLAGS) -o ../bin/$(HOST)/$(TARGET)/$(shell basename $@) $<

test: clean $(TARGET)

clean:
	@rm -rf ./*.o $(TARGET) ../bin/$(HOST)/$(TARGET)

$(TARGET): .bin  $(OBJECTS_C) $(OBJECTS_CPP)
	@$(CPP) ${CFLAGS} -o ../bin/$(HOST)/$(TARGET)/$(shell basename $@) ../bin/$(HOST)/$(TARGET)/*.o $(LDFLAGS) $(LDEXTRA)
	@cp $(CFGNAME) ../bin/$(HOST)/$(TARGET)/proxyJSON.conf
	@rm ../bin/$(HOST)/$(TARGET)/*.o