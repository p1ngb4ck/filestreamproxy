## Process this file with automake to produce Makefile.in

bin_PROGRAMS = transtreamproxy
transtreamproxy_SOURCES = \
	main.cpp \
	Demuxer.cpp \
	Encoder.cpp \
	Http.cpp \
	Logger.cpp \
	Mpeg.cpp \
	UriDecoder.cpp \
	Util.cpp \
    SessionMap.cpp \
	3rdparty/trap.cpp

transtreamproxy_CXXFLAGS = $(LIBSDL_CFLAGS) -Wno-unused-result -D_GNU_SOURCE -D_FILE_OFFSET_BITS=64
transtreamproxy_LDADD = $(LIBLTDL) -lpthread -lrt
