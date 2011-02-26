
AUTOMAKE_OPTIONS = nostdinc

AM_CPPFLAGS = \
	-I$(top_builddir)/include \
	-I$(top_srcdir)/include \
	-I$(includedir)

lib_LTLIBRARIES = libqsescm.la 

libqsescm_la_SOURCES = scm.h scm.c print.c err.c
libqsescm_la_LDFLAGS = -L../cmn -L$(libdir) -version-info 1:0:0 -no-undefined
libqsescm_la_LIBADD = -lqsecmn
