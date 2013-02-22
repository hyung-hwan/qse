AUTOMAKE_OPTIONS = nostdinc

AM_CPPFLAGS = \
	-I$(top_builddir)/include \
	-I$(top_srcdir)/include \
	-I$(includedir)


qsecfgdir = $(sysconfdir)/qse
qsecfg_DATA = httpd.conf httpd-mime.conf

bin_PROGRAMS = qsehttpd

qsehttpd_SOURCES = httpd.c
qsehttpd_LDFLAGS = -L../../lib/xli -L../../lib/http -L../../lib/cmn -L$(libdir)
qsehttpd_LDADD = -lqsexli -lqsehttp -lqsecmn

if WIN32 
if WCHAR
qsehttpd_LDADD += $(UNICOWS_LIBS)
endif
endif

EXTRA_DIST = $(qsecfg_DATA)
