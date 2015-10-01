AUTOMAKE_OPTIONS = nostdinc

AM_CPPFLAGS = \
	-I$(top_builddir)/include \
	-I$(top_srcdir)/include \
	-I$(includedir)

bin_PROGRAMS = \
	rwl01
	
AM_LDFLAGS = -L../../lib/sys  -L../../lib/cmn
AM_CFLAGS = $(PTHREAD_CFLAGS)
LDADD = -lqsesys -lqsecmn $(PTHREAD_LIBS)

if WIN32 
if WCHAR
LDADD += $(UNICOWS_LIBS)
endif
endif

rwl01_SOURCES = rwl01.c
