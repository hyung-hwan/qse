
AM_CPPFLAGS = \
	-I$(top_srcdir)/include \
	-I$(includedir)

bin_PROGRAMS = qsescm 

qsescm_SOURCES = scm.c
qsescm_LDFLAGS = -L../../lib/scm  -L$(libdir)
qsescm_LDADD = -lqsescm $(LIBM)

