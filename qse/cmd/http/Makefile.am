AUTOMAKE_OPTIONS = nostdinc

AM_CPPFLAGS = \
	-I$(top_builddir)/include \
	-I$(top_srcdir)/include 


qsecfgdir = $(sysconfdir)/qse
qsecfg_DATA = httpd.conf httpd-mime.conf

bin_PROGRAMS = qsehttpd qseursd

qsehttpd_SOURCES = httpd.c
qsehttpd_CFLAGS = $(PTHREAD_CFLAGS)
qsehttpd_LDFLAGS = -L../../lib/xli -L../../lib/http -L../../lib/si -L../../lib/cmn  
qsehttpd_LDADD = -lqsexli -lqsehttp -lqsesi -lqsecmn $(SSL_LIBS) $(LIBM) $(PTHREAD_LIBS)
qsehttpd_DEPENDENCIES = ../../lib/xli/libqsexli.la ../../lib/http/libqsehttp.la ../../lib/si/libqsesi.la ../../lib/cmn/libqsecmn.la

if WIN32 
qsehttpd_LDADD += $(UNICOWS_LIBS)
endif

qseursd_SOURCES = ursd.c
qseursd_CFLAGS = $(PTHREAD_CFLAGS)
qseursd_LDFLAGS = -L../../lib/si -L../../lib/cmn
qseursd_LDADD = -lqsesi -lqsecmn $(LIBM) $(PTHREAD_LIBS)
qseursd_DEPENDENCIES = ../../lib/si/libqsesi.la ../../lib/cmn/libqsecmn.la

EXTRA_DIST = $(qsecfg_DATA)
