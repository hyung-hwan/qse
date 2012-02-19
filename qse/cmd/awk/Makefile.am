AUTOMAKE_OPTIONS = nostdinc

AM_CPPFLAGS = \
	-I$(top_builddir)/include \
	-I$(top_srcdir)/include \
	-I$(includedir)

bin_PROGRAMS = qseawk

qseawk_SOURCES = awk.c
qseawk_LDFLAGS = -L../../lib/awk -L../../lib/cmn -L$(libdir)
qseawk_LDADD = -lqseawk -lqsecmn $(LIBM)

if WIN32 
qseawk_LDADD += $(UNICOWS_LIBS)
endif
