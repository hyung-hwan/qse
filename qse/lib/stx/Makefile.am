
AUTOMAKE_OPTIONS = nostdinc

AM_CPPFLAGS = \
	-I$(top_builddir)/include \
	-I$(top_srcdir)/include \
	-I$(includedir)

lib_LTLIBRARIES = libqsestx.la 

libqsestx_la_SOURCES = stx.c err.c mem.c obj.c sym.c boot.c misc.c
libqsestx_la_LDFLAGS = -L../cmn -L$(libdir) -version-info 1:0:0 -no-undefined
libqsestx_la_LIBADD = -lqsecmn
