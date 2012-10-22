AUTOMAKE_OPTIONS = nostdinc

AM_CPPFLAGS = \
	-I$(top_builddir)/include \
	-I$(top_srcdir)/include \
	-I$(includedir)

moddir = $(libdir)/qseawk
mod_LTLIBRARIES = libsys.la libuci.la

libsys_la_SOURCES = sys.c
libsys_la_LDFLAGS = -L$(abs_builddir)/../../lib/cmn -L$(abs_builddir)/../../lib/awk -L$(libdir) -version-info 1:0:0 -no-undefined
libsys_la_LIBADD = -lqseawk -lqsecmn 

libuci_la_SOURCES = uci.c
libuci_la_LDFLAGS = -L$(abs_builddir)/../../lib/cmn -L$(abs_builddir)/../../lib/awk -L$(libdir) -version-info 1:0:0 -no-undefined
libuci_la_LIBADD = -lqseawk -lqsecmn 
