# -*- makefile -*-
#
#	makefile for http communication with PPC web server
#
# @author Yvan Castilloux

include ../../support/make/Makefile.include

LIB_NAME = libhttpcomm
SOURCE_NAME = libhttpcomm
SOURCES = $(SOURCE_NAME).c
#RESULT_DIR = ./
RESULT_DIR = .
CFLAGS += -I../../include

# Include 3rd party libraries
#CFLAGS += -I../../lib/3rdparty/${LIBXML2_VERSION}/include
#CFLAGS += -I../../lib/3rdparty/${LIBCURL_VERSION}/include
#CFLAGS += -I../../lib/3rdparty/${OPENSSL_VERSION}/include
ifneq ($(LIBXML2_VERSION), )
CFLAGS += -I../../lib/3rdparty/${LIBXML2_VERSION}/include
else
ifeq ($(CONFIG_BUILD_IN_TARGET), 1)
CFLAGS += -I/usr/include/libxml2
endif
endif
ifneq ($(LIBCURL_VERSION), )
CFLAGS += -I../../lib/3rdparty/${LIBCURL_VERSION}/include
else
ifeq ($(CONFIG_BUILD_IN_TARGET), 1)
CFLAGS += -I/usr/include/curl
endif
endif
ifneq ($(OPENSSL_VERSION), )
CFLAGS += -I../../lib/3rdparty/${OPENSSL_VERSION}/include
else
ifeq ($(CONFIG_BUILD_IN_TARGET), 1)
CFLAGS += -I/usr/include/openssl
endif
endif
CFLAGS += -I../../lib/3rdparty/${CJSON_VERSION}

#ifneq ($(HOST), mips-linux)
#CFLAGS+= -g -pg
#endif
ARFLAG = rcs

CFLAGS += -Wall

DYNLIB_EXTENSION = so
STATLIB_EXTENSION = a

# Link Flag
#LINK_FLAG = -L$(RESULT_DIR) -shared -Wl,-soname,$(LIB_NAME).$(DYNLIB_EXTENSION) -lc -lm -lpthread

LINK_FLAG = -shared -o $(RESULT_DIR)/$(LIB_NAME).so $(OBJECTS)

OBJECTS=$(SOURCES:.c=.o)
LDEXTRA+=$(PPCLIBPATH) $(LIBRT) $(LIBXML2) $(LIBCURL) -lpthread
LOCALINCLUDEPATH =

all: dynlib staticlib

clean:
	$(RM) -rf ./*.o ./*.d ./*.dll ./*.a ../*.a ./*.so ../*.so ../../include/$(SOURCE_NAME).h $(LIB_NAME)

$(LIB_NAME): $(OBJECTS)
	$(CC) $(PPCINCLUDEPATH) $(LOCALINCLUDEPATH) $(LDFLAGS) -o $@ $(OBJECTS) $(LDEXTRA)

.c.o:
	$(CC) $(PPCINCLUDEPATH) $(LOCALINCLUDEPATH) $(CFLAGS) -c -o $@ $<

staticlib: $(OBJECTS)
	$(AR) $(ARFLAG) $(RESULT_DIR)/$(LIB_NAME).$(STATLIB_EXTENSION) ${OBJECTS}
	@cp ./$(LIB_NAME).a ../$(LIB_NAME).a
	@mkdir -p ../../include
	@cp ./$(SOURCE_NAME).h ../../include/.

dynlib: $(OBJECTS)
	$(CC) $(LINK_FLAG) $(LDEXTRA)
	@cp ./$(LIB_NAME).so ../
	@mkdir -p ../../include
	@cp ./$(SOURCE_NAME).h ../../include/.
