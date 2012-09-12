AUTOMAKE_OPTIONS = nostdinc

AM_CPPFLAGS = \
	-I$(top_builddir)/include \
	-I$(top_srcdir)/include \
	-I$(includedir) 
	
bin_PROGRAMS = httpd01 httpd02 upxd01

LDFLAGS += -L../../lib/cmn -L../../lib/net 
LDADD = -lqsenet -lqsecmn $(PTHREAD_LIBS) $(SOCKET_LIBS) $(SENDFILE_LIBS)

if WIN32 
if WCHAR
LDADD += $(UNICOWS_LIBS)
endif
endif

httpd01_SOURCES = httpd01.c
httpd02_SOURCES = httpd02.c
upxd01_SOURCES = upxd01.c

httpd01_LDADD = $(LDADD) $(SSL_LIBS)
httpd02_LDADD = $(LDADD) $(SSL_LIBS)
