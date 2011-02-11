AUTOMAKE_OPTIONS = nostdinc

AM_CPPFLAGS = \
	-I$(top_builddir)/include \
	-I$(top_srcdir)/include \
	-I$(includedir)

bin_PROGRAMS = http01 

LDFLAGS = -L../../lib/cmn -L../../lib/http
LDADD = -lqsehttp -lqsecmn $(LIBM)

http01_SOURCES = http01.c

