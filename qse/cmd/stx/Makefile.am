AUTOMAKE_OPTIONS = nostdinc

AM_CPPFLAGS = \
	-I$(top_builddir)/include \
	-I$(top_srcdir)/include \
	-I$(includedir)

bin_PROGRAMS = qsestx 

qsestx_SOURCES = stx.c
qsestx_LDFLAGS = -L../../lib/stx -L../../lib/cmn -L$(libdir)
qsestx_LDADD = -lqsestx -lqsecmn $(LIBM)

