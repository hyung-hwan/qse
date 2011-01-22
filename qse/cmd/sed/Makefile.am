
AM_CPPFLAGS = \
	-I$(top_srcdir)/include \
	-I$(includedir)

bin_PROGRAMS = qsesed

qsesed_SOURCES = sed.c
qsesed_LDFLAGS = -L../../lib/sed -L$(libdir)
qsesed_LDADD = -lqsesed 

