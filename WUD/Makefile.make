# -*- makefile -*-
#
#	makefile for writing configurations into a file
#
# @author mlevitin

include ../support/make/Makefile.include

# What is the main file we want to compile
TARGET = WUD

SOURCES_C = main.c
SOURCES_C += ./wud_actions/wa_alarm.c
SOURCES_C += ./wud_actions/wa_manage_children.c
SOURCES_C += ./wud_actions/wa_reboot.c
SOURCES_C += ./wud_config/wc_settings.c
SOURCES_C += ./wud_fw_upgrade/wf_upgrade.c
SOURCES_C += ./wud_http/wh_manager.c
SOURCES_C += ./wud_monitor/wm_childs_info.c
SOURCES_C += ./wud_monitor/wm_monitor.c
SOURCES_C += ./wud_threads/wt_agent_proxy_read.c
SOURCES_C += ./wud_threads/wt_fw_upgrade.c
SOURCES_C += ./wud_threads/wt_monitor.c
SOURCES_C += ./wud_threads/wt_queues.c
SOURCES_C += ./wud_threads/wt_server_write.c
SOURCES_C += ./wud_threads/wt_threads.c
SOURCES_C += ./wud_threads/wt_watchdog.c
SOURCES_C += ./wud_utils/wu_utils.c

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
SOURCES_C += ../lib/libsha/lib_sha_256.c


# Which test(s) are we trying to run
SOURCES_CPP =

# Where are all of our directories we should include
CFLAGS += -I./
CFLAGS += -I./wud_actions
CFLAGS += -I./wud_config
CFLAGS += -I./wud_fw_upgrade
CFLAGS += -I./wud_http
CFLAGS += -I./wud_monitor
CFLAGS += -I./wud_threads
CFLAGS += -I./wud_utils
CFLAGS += -I../lib/libtimer
CFLAGS += -I../lib/presto_commands
CFLAGS += -I../lib/libtcp
CFLAGS += -I../lib/pu_logger
CFLAGS += -I../lib/pc_config
CFLAGS += -I../lib/pu_queues
CFLAGS += -I../lib/libhttpcomm
CFLAGS += -I../lib/json
CFLAGS += -I../lib/libsha

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

LDEXTRA += -lcurl -lpthread -lm
ifeq ($(HOST), mips-linux)
    LDEXTRA+=-lmbedtls
    LDFLAGS += -Wl,-rpath $(ROOTFS_PATH)/usr/lib
    CFGNAME=wud.conf.mips
endif

CFLAGS += -Os
CFLAGS += -Wall


all:	$(TARGET)

.bin:
	@mkdir -p ../bin/$(HOST)/$(TARGET)

.c.o:
	@$(CC) -c $(CFLAGS) -o ../bin/$(HOST)/$(TARGET)/$(shell basename $@) $<

.cpp.o:
	@mkdir ./bin
	@$(CPP) -c $(CFLAGS) -o ../bin/$(HOST)/$(TARGET)/$(shell basename $@) $<

test: clean $(TARGET)

clean:
	@rm -rf ./*.o $(TARGET) ../bin/$(HOST)/$(TARGET)

$(TARGET): .bin  $(OBJECTS_C) $(OBJECTS_CPP)
	@$(CPP) ${CFLAGS} -o ../bin/$(HOST)/$(TARGET)/$(shell basename $@) ../bin/$(HOST)/$(TARGET)/*.o $(LDFLAGS) $(LDEXTRA)
	@cp $(CFGNAME) ../bin/$(HOST)/$(TARGET)/wud.conf
	@rm ../bin/$(HOST)/$(TARGET)/*.o

