
AUTOMAKE_OPTIONS = nostdinc

AM_CPPFLAGS = \
	-I$(top_builddir)/include \
	-I$(top_srcdir)/include 

lib_LTLIBRARIES = libqsestx.la 

libqsestx_la_SOURCES = \
	stx.h hash.h mem.h obj.h sym.h dic.h cls.h boot.h \
	stx.c err.c hash.c mem.c obj.c sym.c dic.c cls.c boot.c par.c
libqsestx_la_LDFLAGS = -L../cmn -version-info 1:0:0 -no-undefined
libqsestx_la_LIBADD = -lqsecmn
