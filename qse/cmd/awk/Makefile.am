AUTOMAKE_OPTIONS = nostdinc

AM_CPPFLAGS = \
	-I$(top_builddir)/include \
	-I$(top_srcdir)/include \
	-I$(includedir)

#####################################################################3

bin_PROGRAMS = qseawk

qseawk_SOURCES = awk.c
qseawk_LDFLAGS = -L../../lib/awk -L../../lib/cmn -L$(libdir)
qseawk_LDADD = -lqseawk -lqsecmn $(LIBM)

if WIN32 
qseawk_LDADD += $(UNICOWS_LIBS)
endif

#####################################################################3

if HAVE_C_MPI
bin_PROGRAMS += qseawkp

qseawkp_SOURCES = mpi.c
qseawkp_CPPFLAGS = $(AM_CPPFLAGS) $(MPI_CFLAGS)
qseawkp_LDFLAGS = -L../../lib/awk -L../../lib/cmn -L$(libdir) $(MPI_CLDFLAGS)
qseawkp_LDADD = -lqseawkp -lqseawk -lqsecmn $(LIBM)

if WIN32 
qseawkp_LDADD += $(UNICOWS_LIBS)
endif

endif
