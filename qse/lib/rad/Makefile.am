AUTOMAKE_OPTIONS = nostdinc

AM_CPPFLAGS = \
	-I$(top_builddir)/include \
	-I$(top_srcdir)/include

#noinst_HEADERS =

lib_LTLIBRARIES = libqserad.la
libqserad_la_SOURCES =  \
	raddic.c \
	radmsg.c

libqserad_la_CFLAGS = 
libqserad_la_LDFLAGS = -L../cmn -L../si -L../cry -version-info 1:0:0 -no-undefined
libqserad_la_LIBADD = -lqsecry -lqsesi -lqsecmn
libqserad_la_DEPENDENCIES = ../../lib/cry/libqsecry.la  ../../lib/si/libqsesi.la ../../lib/cmn/libqsecmn.la
