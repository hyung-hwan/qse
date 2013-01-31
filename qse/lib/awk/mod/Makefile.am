AUTOMAKE_OPTIONS = nostdinc

AM_CPPFLAGS = \
	-I$(top_builddir)/include \
	-I$(top_srcdir)/include \
	-I$(includedir)

LDFLAGS_COMMON = -L$(abs_builddir)/../../cmn -L$(abs_builddir)/../../awk -L$(libdir) -no-undefined -version-info 1:0:0
LIBADD_COMMON = -lqseawk -lqsecmn

moddir = $(libdir)

mod_LTLIBRARIES = libqseawk-sys.la 
libqseawk_sys_la_SOURCES = sys.c
libqseawk_sys_la_LDFLAGS = $(LDFLAGS_COMMON)
libqseawk_sys_la_LIBADD = $(LIBADD_COMMON)

mod_LTLIBRARIES += libqseawk-str.la 
libqseawk_str_la_SOURCES = str.c
libqseawk_str_la_LDFLAGS = $(LDFLAGS_COMMON)
libqseawk_str_la_LIBADD = $(LIBADD_COMMON)

mod_LTLIBRARIES += libqseawk-dir.la 
libqseawk_dir_la_SOURCES = dir.c
libqseawk_dir_la_LDFLAGS = $(LDFLAGS_COMMON)
libqseawk_dir_la_LIBADD = $(LIBADD_COMMON)

if HAVE_LIBUCI
mod_LTLIBRARIES += libqseawk-uci.la
libqseawk_uci_la_SOURCES = uci.c
libqseawk_uci_la_LDFLAGS = $(LDFLAGS_COMMON)
libqseawk_uci_la_LIBADD = $(LIBADD_COMMON) $(UCI_LIBS)
endif

if HAVE_C_MPI
mod_LTLIBRARIES += libqseawk-mpi.la
libqseawk_mpi_la_CPPFLAGS = $(AM_CPPFLAGS) $(MPI_CFLAGS)
libqseawk_mpi_la_SOURCES = mpi.c
libqseawk_mpi_la_LDFLAGS = $(LDFLAGS_COMMON) $(MPI_CLDFLAGS)
libqseawk_mpi_la_LIBADD = $(LIBADD_COMMON)
endif
