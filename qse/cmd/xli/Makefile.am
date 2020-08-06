AUTOMAKE_OPTIONS = nostdinc

AM_CPPFLAGS = \
	-I$(top_builddir)/include \
	-I$(top_srcdir)/include 

bin_PROGRAMS = qsexli

qsexli_SOURCES = xli.c
qsexli_CFLAGS = $(PTHREAD_CFLAGS)
qsexli_LDFLAGS = -L../../lib/xli -L../../lib/si -L../../lib/cmn 
qsexli_LDADD = -lqsexli -lqsesi -lqsecmn $(LIBM) $(PTHREAD_LIBS)
qsexli_DEPENDENCIES = ../../lib/xli/libqsexli.la ../../lib/si/libqsesi.la ../../lib/cmn/libqsecmn.la

if WIN32 
if WCHAR
qsexli_LDADD += $(UNICOWS_LIBS)
endif
endif
