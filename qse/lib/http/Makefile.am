AUTOMAKE_OPTIONS = nostdinc

CPPFLAGS_ALL_COMMON = \
	-I$(top_builddir)/include \
	-I$(top_srcdir)/include

if WIN32
# you must adjust the value of DEFAULT_MODPOSTFIX according 
# to the first number in -version-info below
CPPFLAGS_HTTPD_MOD = -DQSE_HTTPD_DEFAULT_MODPREFIX=\"libqsehttpd-\" -DQSE_HTTPD_DEFAULT_MODPOSTFIX=\"-1\"
else
CPPFLAGS_HTTPD_MOD = -DQSE_HTTPD_DEFAULT_MODPREFIX=\"$(libdir)/libqsehttpd-\" -DQSE_HTTPD_DEFAULT_MODPOSTFIX=\"\"
endif

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
	httpd-std.c \
	httpd-std-dns.h \
	httpd-std-mod.h \
	httpd-std-urs.h \
	httpd-task.c \
	httpd-text.c \
	upxd.c

libqsehttp_la_CPPFLAGS = $(CPPFLAGS_ALL_COMMON) $(CPPFLAGS_HTTPD_MOD) $(LTDLINCL)
libqsehttp_la_LDFLAGS = -L../cmn -version-info 1:0:0 -no-undefined 
libqsehttp_la_LIBADD = -lqsecmn $(SOCKET_LIBS) $(SENDFILE_LIBS) $(SSL_LIBS) $(LIBLTDL)

