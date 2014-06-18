#============================================================================
# Name        : Makefile (transtreamproxy)
# Author      : oskwon(kos@dev3)
# Version     : 
# Copyright   : Copyright(c)2013 Vu+ Team. All right reserved.
# Description :
#============================================================================

-include ../config.mk

ifeq ($(MODEL),)
$(error config.mk is not set. please run script.config before make.)
endif

MAJOR = 3
MINOR = 0
PROJECT = transtreamproxy

TOP=$(PWD)/..

CROSS=$(OETOP)/$(MODEL)/build/tmp/cross/mipsel/bin/mipsel-oe-linux-
SYSROOT=$(OETOP)/$(MODEL)/build/tmp/staging/mipsel-oe-linux

RM=rm -f
CXX=$(CROSS)g++
LD=$(CROSS)ld
STRIP=$(CROSS)strip
UPLOAD=$(TOP)/script/script.upload

SRCS = $(shell find . -name "*.cpp")
OBJS=$(SRCS:.cpp=.o)

CFLAGS += -D_MAJOR=$(MAJOR) -D_MINOR=$(MINOR) 
CFLAGS += -O2 -D_GNU_SOURCE -D_FILE_OFFSET_BITS=64 -I. -I./external -I$(SYSROOT)/usr/include 
LDFLAGS += -L$(SYSROOT)/usr/lib -lpthread -lrt

.SUFFIXES : .cpp .o
.PHONY : all clean install .showinfo

.cpp.o: 
	$(CXX) -c $(CFLAGS) -o $@ $<

all: .showinfo $(PROJECT)

$(PROJECT):$(OBJS)
	$(CXX) -o $@ $(OBJS) $(LDFLAGS)
	$(STRIP) $@

install:
	@$(UPLOAD) $(IP) . $(PROJECT) /usr/bin

clean:
	$(RM) $(PROJECT) $(OBJS) *.log 

.showinfo:
	@echo "-----------------------------------------------------"
	@echo "                [ BUILD ENVIRONMENT ]                "
	@echo "-----------------------------------------------------"
	@echo "PROJECT  : "$(PROJECT)" (v"$(MAJOR)"."$(MINOR)")"
	@echo ""
	@echo "CXX      : "$(CXX)
	@echo "LD       : "$(LD)
	@echo "STRIP    : "$(STRIP)
	@echo "CFLAGS   : "$(CFLAGS)
	@echo "LDFLAGS  : "$(LDFLAGS)
	@echo "-----------------------------------------------------"
	@echo
