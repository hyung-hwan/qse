AUTOMAKE_OPTIONS = nostdinc

AM_CPPFLAGS = \
	-I$(top_builddir)/include \
	-I$(top_srcdir)/include \
	-I$(includedir) 
	
bin_PROGRAMS = http01 upxd01

LDFLAGS += -L../../lib/cmn -L../../lib/net 
LDADD = -lqsenet -lqsecmn $(PTHREAD_LIBS) $(SOCKET_LIBS) $(SENDFILE_LIBS) -lssl

if WIN32 
LDADD += $(UNICOWS_LIBS)
endif

http01_SOURCES = http01.c
upxd01_SOURCES = upxd01.c

