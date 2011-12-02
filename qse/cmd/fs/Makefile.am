AUTOMAKE_OPTIONS = nostdinc

AM_CPPFLAGS = \
	-I$(top_builddir)/include \
	-I$(top_srcdir)/include \
	-I$(includedir)

bin_PROGRAMS = qsefs

qsefs_SOURCES = fs.c
qsefs_LDFLAGS = -L../../lib/cmn -L$(libdir)
qsefs_LDADD = -lqsecmn

