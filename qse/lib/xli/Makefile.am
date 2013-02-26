AUTOMAKE_OPTIONS = nostdinc

AM_CPPFLAGS = \
	-I$(top_builddir)/include \
	-I$(top_srcdir)/include 

lib_LTLIBRARIES = libqsexli.la
libqsexli_la_SOURCES = xli.h xli.c err.c read.c write.c std.c
libqsexli_la_LDFLAGS = -L../cmn -version-info 1:0:0 -no-undefined
libqsexli_la_LIBADD = -lqsecmn 

