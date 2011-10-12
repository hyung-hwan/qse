AUTOMAKE_OPTIONS = nostdinc

AM_CPPFLAGS = \
	-I$(top_builddir)/include \
	-I$(top_srcdir)/include \
	-I$(includedir)

lib_LTLIBRARIES = libqsefs.la
libqsefs_la_SOURCES = \
	dir.c

libqsefs_la_LDFLAGS = -version-info 1:0:0 -no-undefined -L../cmn -L$(libdir)
libqsefs_la_LIBADD = -lqsecmn
