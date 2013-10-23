## Process this file with automake to produce Makefile.in
bin_PROGRAMS = filestreamproxy
filestreamproxy_SOURCES =  main.cpp
filestreamproxy_SOURCES += eFilePumpThread.cpp
filestreamproxy_SOURCES += eNetworkPumpThread.cpp
filestreamproxy_SOURCES += eURIDecoder.cpp
filestreamproxy_SOURCES += uThread.cpp

filestreamproxy_LDADD = $(LIBLTDL) @PTHREAD_LIBS@


