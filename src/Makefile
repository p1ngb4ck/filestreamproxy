#============================================================================
# Name        : Makefile
# Author      : oskwon(kos@dev3)
# Version     : 
# Copyright   : Copyright(c)2013 Vu+ Team. All right reserved.
# Description :
#============================================================================

VER = v3.0


Q=@
IP=100.214
MODEL=vuduo2
OETOP=/home/oskwon/works/workrounds/openembedded
CROSS=$(OETOP)/$(MODEL)/build/tmp/cross/mipsel/bin/mipsel-oe-linux-
SYSROOT=$(OETOP)/$(MODEL)/build/tmp/staging/mipsel-oe-linux

BIN = transtreamproxy
SRCS = $(shell find . -name "*.cpp")

CFLAGS ?=
LDFLAGS ?=

#CFLAGS += -Wno-unused-result

CFLAGS += -O2

CXX=$(CROSS)g++
LD=$(CROSS)ld
STRIP=$(CROSS)strip
RM=rm -f

CFLAGS += -I. -I./openpli-streamproxy -I$(SYSROOT)/usr/include 
LDFLAGS += -L$(SYSROOT)/usr/lib -lpthread -lrt

OBJS=$(SRCS:.cpp=.o)

.SUFFIXES : .cpp .o
.PHONY : all clean install .showinfo

.cpp.o: 
	$(Q)echo "Compile... "$<
	$(Q)$(CXX) -c $(CFLAGS) -o $@ $<

all: .showinfo $(BIN)

$(BIN):$(OBJS)
	$(Q)echo "Linking... "$@
	$(Q)$(CXX) -o $@ $(OBJS) $(LDFLAGS)
	$(Q)$(STRIP) $@

install:
	$(Q)./up.sh $(IP)

clean:
	$(Q)$(RM) $(BIN) $(OBJS) *.log 

.showinfo:
	@echo "-----------------------------------------------------"
	@echo "                [ BUILD ENVIRONMENT ]                "
	@echo "-----------------------------------------------------"
	@echo "OUTPUT   : "$(STREAMER_BIN) $(TRANSTREAMPROXY_BIN) 
	@echo "VERSION  : "$(VER)
	@echo ""
	@echo "CXX      : "$(CXX)
	@echo "LD       : "$(LD)
	@echo "STRIP    : "$(STRIP)
	@echo "CFLAGS   : "$(CFLAGS)
	@echo "LDFLAGS  : "$(LDFLAGS)
	@echo "-----------------------------------------------------"
	@echo
