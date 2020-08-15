AUTOMAKE_OPTIONS = nostdinc

AM_CPPFLAGS = \
	-I$(top_builddir)/include \
	-I$(top_srcdir)/include 

bin_PROGRAMS = qsesed

qsesed_SOURCES = sed.c
qsesed_CFLAGS = $(PTHREAD_CFLAGS)
qsesed_LDFLAGS = -L../../lib/sed -L../../lib/si -L../../lib/cmn 
qsesed_LDADD = -lqsesed -lqsesi -lqsecmn $(PTHREAD_LIBS)
qsesed_DEPENDENCIES = ../../lib/sed/libqsesed.la ../../lib/si/libqsesi.la ../../lib/cmn/libqsecmn.la

if WIN32 
qsesed_LDADD += $(UNICOWS_LIBS)
endif
