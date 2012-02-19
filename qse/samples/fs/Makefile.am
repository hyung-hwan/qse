AUTOMAKE_OPTIONS = nostdinc

AM_CPPFLAGS = \
	-I$(top_builddir)/include \
	-I$(top_srcdir)/include \
	-I$(includedir) 
	
bin_PROGRAMS = \
	dir01

LDFLAGS += -L../../lib/cmn -L../../lib/fs
LDADD = -lqsefs -lqsecmn

if WIN32 
LDADD += $(UNICOWS_LIBS)
endif

dir01_SOURCES = dir01.c

