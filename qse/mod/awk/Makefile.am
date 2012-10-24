AUTOMAKE_OPTIONS = nostdinc

AM_CPPFLAGS = \
	-I$(top_builddir)/include \
	-I$(top_srcdir)/include \
	-I$(includedir)

moddir = $(libdir)/qse
mod_LTLIBRARIES = libawksys.la 

libawksys_la_SOURCES = sys.c
libawksys_la_LDFLAGS = -L$(abs_builddir)/../../lib/cmn -L$(abs_builddir)/../../lib/awk -L$(libdir) -version-info 1:0:0 -no-undefined
libawksys_la_LIBADD = -lqseawk -lqsecmn 

if HAVE_LIBUCI

mod_LTLIBRARIES += libawkuci.la
libawkuci_la_SOURCES = uci.c
libawkuci_la_LDFLAGS = -L$(abs_builddir)/../../lib/cmn -L$(abs_builddir)/../../lib/awk -L$(libdir) -version-info 1:0:0 -no-undefined
libawkuci_la_LIBADD = -lqseawk -lqsecmn $(UCI_LIBS)

endif
