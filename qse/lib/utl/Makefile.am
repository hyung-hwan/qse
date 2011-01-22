
AM_CPPFLAGS = -I$(top_srcdir)/include -I$(includedir)

lib_LTLIBRARIES = libqseutl.la
libqseutl_la_SOURCES = \
	http.c 

libqseutl_la_LDFLAGS = -version-info 1:0:0 -no-undefined -L../cmn -L$(libdir)
libqseutl_la_LIBADD = -lqsecmn
