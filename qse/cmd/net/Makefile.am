AUTOMAKE_OPTIONS = nostdinc

AM_CPPFLAGS = \
	-I$(top_builddir)/include \
	-I$(top_srcdir)/include \
	-I$(includedir)

bin_PROGRAMS = qsehttpd

qsehttpd_SOURCES = httpd.c
qsehttpd_LDFLAGS = -L../../lib/net -L../../lib/cmn -L$(libdir)
qsehttpd_LDADD = -lqsenet -lqsecmn

if WIN32 
if WCHAR
qsehttpd_LDADD += $(UNICOWS_LIBS)
endif
endif
