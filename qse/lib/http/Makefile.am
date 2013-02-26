AUTOMAKE_OPTIONS = nostdinc

AM_CPPFLAGS = \
	-I$(top_builddir)/include \
	-I$(top_srcdir)/include

lib_LTLIBRARIES = libqsehttp.la
libqsehttp_la_SOURCES = \
	httpd.h \
	upxd.h \
	http.c \
	htre.c \
	htrd.c \
	httpd.c \
	httpd-cgi.c \
	httpd-dir.c \
	httpd-file.c \
	httpd-proxy.c \
	httpd-resol.c \
	httpd-std.c \
	httpd-task.c \
	httpd-text.c \
	upxd.c

libqsehttp_la_LDFLAGS = -L../cmn -version-info 1:0:0 -no-undefined
libqsehttp_la_LIBADD = -lqsecmn $(SOCKET_LIBS) $(SENDFILE_LIBS) $(SSL_LIBS)

