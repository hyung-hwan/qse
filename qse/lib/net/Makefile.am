AUTOMAKE_OPTIONS = nostdinc

AM_CPPFLAGS = \
	-I$(top_builddir)/include \
	-I$(top_srcdir)/include \
	-I$(includedir)

lib_LTLIBRARIES = libqsenet.la
libqsenet_la_SOURCES = \
	http.c \
	htre.c \
	htrd.c \
	httpd.h \
	httpd.c \
	httpd-cgi.c \
	httpd-proxy.c \
	httpd-resol.c \
	httpd-task.c \
	upxd.c

libqsenet_la_LDFLAGS = -version-info 1:0:0 -no-undefined -L../cmn -L$(libdir)
libqsenet_la_LIBADD = -lqsecmn
