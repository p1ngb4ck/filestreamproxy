#============================================================================
# Name        : Makefile (transtreamproxy)
# Author      : oskwon(oskwon@dev3)
# Version     : 
# Copyright   : Copyright(c)2014 Vu+ Team. All right reserved.
# Description :
#============================================================================

-include ../build/config.mk

ifeq ($(MODEL),)
$(error config.mk is not set. please run script.config before make.)
endif

MAJOR = 3
MINOR = 0

TOP=$(PWD)/..
OBJ=./obj/

ifeq ($(CROSS),YES)
SYSROOT=$(OETOP)/build/$(MODEL)/tmp/sysroots/$(MODEL)
TOOLCHAIN=$(OETOP)/build/$(MODEL)/tmp/sysroots/i686-linux/usr/bin/mips32el-oe-linux/mipsel-oe-linux-
endif

RM=rm -Rf
CXX=$(TOOLCHAIN)g++
LD=$(TOOLCHAIN)ld
STRIP=$(TOOLCHAIN)strip
UPLOAD=./script.upload

ifeq ($(MODE),DEBUG)
CFLAGS += -g
else
CFLAGS += -O2
endif
CFLAGS += -D_MAJOR=$(MAJOR) -D_MINOR=$(MINOR) -D_GNU_SOURCE -D_FILE_OFFSET_BITS=64
CFLAGS += -I$(SYSROOT)/usr/include
LDFLAGS += -L$(SYSROOT)/usr/lib -lpthread -lrt

SRCS = ../src/3rdparty/trap.cpp
SRCS+= ../src/Demuxer.cpp
SRCS+= ../src/Encoder.cpp
SRCS+= ../src/Http.cpp
SRCS+= ../src/Logger.cpp
SRCS+= ../src/Mpeg.cpp
SRCS+= ../src/UriDecoder.cpp
SRCS+= ../src/Util.cpp

OBJS = $(SRCS:.cpp=.o)
CFLAGS += $(addprefix -I, $(shell find ../src/ -type d))

.SUFFIXES : .cpp .o
.PHONY : all clean install .showinfo .prepare $(PROJECT)

.cpp.o:
	@echo "Compile... ["$<"]"
	@$(CXX) -c $(CFLAGS) -o $(OBJ)$(notdir $@) $<

all: .showinfo .prepare $(OBJS) stress_full

demux_loop: .prepare
	@echo "Link... ["$@"]"
	@$(CXX) $(CFLAGS) -o bin/$@ $@.cpp $(addprefix $(OBJ), $(notdir $(OBJS))) $(LDFLAGS)

stress_full: .prepare
	@echo "Link... ["$@"]"
	@g++ -o bin/$@ $@.cpp

transcoding_file: .prepare
	@echo "Link... ["$@"]"
	@$(CXX) $(CFLAGS) -o bin/$@ $@.cpp $(LDFLAGS)

clean:
	$(RM) $(PROJECT) obj bin *.o *.a *.d *.log

.prepare:
	@if [ ! -e obj ]; then mkdir obj; fi
	@if [ ! -e bin ]; then mkdir bin; fi

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
