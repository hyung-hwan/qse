AUTOMAKE_OPTIONS = nostdinc

AM_CPPFLAGS = \
	-I$(top_builddir)/include \
	-I$(top_srcdir)/include 

lib_LTLIBRARIES = libqsexli.la
libqsexli_la_SOURCES = xli-prv.h xli.c err.c \
	read.c read-ini.c read-json.c \
	write.c write-ini.c write-json.c \
	std.c json.c
libqsexli_la_LDFLAGS = -L../cmn -L../si -version-info 1:0:0 -no-undefined
libqsexli_la_LIBADD = -lqsesi -lqsecmn 
libqsexli_la_DEPENDENCIES = ../../lib/si/libqsesi.la ../../lib/cmn/libqsecmn.la


if ENABLE_CXX
libqsexli_la_SOURCES += SkvEnv.cpp
endif

