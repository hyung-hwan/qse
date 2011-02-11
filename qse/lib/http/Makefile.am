AUTOMAKE_OPTIONS = nostdinc

AM_CPPFLAGS = \
	-I$(top_builddir)/include \
	-I$(top_srcdir)/include \
	-I$(includedir)

lib_LTLIBRARIES = libqsehttp.la
libqsehttp_la_SOURCES = \
	http.c 

libqsehttp_la_LDFLAGS = -version-info 1:0:0 -no-undefined -L../cmn -L$(libdir)
libqsehttp_la_LIBADD = -lqsecmn
