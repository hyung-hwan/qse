AUTOMAKE_OPTIONS = nostdinc

AM_CPPFLAGS = \
	-I$(top_builddir)/include \
	-I$(top_srcdir)/include \
	-I$(includedir)

bin_PROGRAMS = qsecut

qsecut_SOURCES = cut.c
qsecut_LDFLAGS = -L../../lib/cut -L../../lib/cmn -L$(libdir)
qsecut_LDADD = -lqsecut -lqsecmn

