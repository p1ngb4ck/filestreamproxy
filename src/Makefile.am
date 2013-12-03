## Process this file with automake to produce Makefile.in

bin_PROGRAMS = transtreamproxy
transtreamproxy_SOURCES = \
	main.cpp \
	uThread.cpp \
	eParser.cpp \
	eURIDecoder.cpp \
	eUpstreamSocket.cpp \
	eTransCodingDevice.cpp \
	eFilePumpThread.cpp \
	eNetworkPumpThread.cpp \
	eDemuxPumpThread.cpp \
	eHostInfoMgr.cpp

transtreamproxy_CXXFLAGS = $(LIBSDL_CFLAGS) @WARN_CFLAGS@
transtreamproxy_LDADD = $(LIBLTDL) @PTHREAD_LIBS@ @RT_LIBS@
