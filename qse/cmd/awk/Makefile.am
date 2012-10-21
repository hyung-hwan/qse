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
if WCHAR
qseawk_LDADD += $(UNICOWS_LIBS)
endif
endif

#####################################################################3

if HAVE_C_MPI
bin_PROGRAMS += qseawkmp

qseawkmp_SOURCES = mpi.c
qseawkmp_CPPFLAGS = $(AM_CPPFLAGS) $(MPI_CFLAGS)
qseawkmp_LDFLAGS = -L../../lib/awk -L../../lib/cmn -L$(libdir) $(MPI_CLDFLAGS)
qseawkmp_LDADD = -lqseawkmp -lqseawk -lqsecmn $(LIBM)

if WIN32
if WCHAR
qseawkmp_LDADD += $(UNICOWS_LIBS)
endif
endif

endif
