AUTOMAKE_OPTIONS = nostdinc

CPPFLAGS_ALL_COMMON = \
	-I$(top_builddir)/include \
	-I$(top_srcdir)/include \
	-I$(includedir)

LDFLAGS_ALL_COMMON = -L. -L../cmn -L$(libdir) -version-info 1:0:0 -no-undefined

##################################################
# MAIN LIBRARY 
##################################################

CPPFLAGS_LIB_COMMON = $(CPPFLAGS_ALL_COMMON) $(LTDLINCL)
LDFLAGS_LIB_COMMON = $(LDFLAGS_ALL_COMMON) 
LIBADD_LIB_COMMON = -lqsecmn $(LIBM) $(LIBLTDL)

if WIN32
# you must adjust the value of DEFAULT_MODPOSTFIX according to
# -version-info in mod/awk/Makefile.am
CPPFLAGS_LIB_COMMON += -DQSE_AWK_DEFAULT_MODPREFIX=\"libqseawk-\" -DQSE_AWK_DEFAULT_MODPOSTFIX=\"-1\"
else
CPPFLAGS_LIB_COMMON += -DQSE_AWK_DEFAULT_MODPREFIX=\"$(libdir)/libqseawk-\" -DQSE_AWK_DEFAULT_MODPOSTFIX=\"\"
endif

lib_LTLIBRARIES = libqseawk.la
libqseawk_la_SOURCES = awk.c err.c tree.c parse.c run.c rec.c val.c fnc.c misc.c rio.c std.c awk.h err.h rio.h val.h fnc.h misc.h parse.h run.h tree.h 
libqseawk_la_CPPFLAGS = $(CPPFLAGS_LIB_COMMON)
libqseawk_la_LDFLAGS = $(LDFLAGS_LIB_COMMON)
libqseawk_la_LIBADD = $(LIBADD_LIB_COMMON)

if ENABLE_CXX
lib_LTLIBRARIES += libqseawkxx.la
libqseawkxx_la_SOURCES = Awk.cpp StdAwk.cpp
libqseawkxx_la_CPPFLAGS = $(CPPFLAGS_LIB_COMMON)
libqseawkxx_la_LDFLAGS = $(LDFLAGS_LIB_COMMON)
libqseawkxx_la_LIBADD = -lqseawk -lqsecmnxx $(LIBADD_LIB_COMMON)
endif

##################################################
# MODULES 
##################################################

CPPFLAGS_MOD_COMMON = $(CPPFLAGS_ALL_COMMON)
LDFLAGS_MOD_COMMON = $(LDFLAGS_ALL_COMMON)
LIBADD_MOD_COMMON = -lqseawk -lqsecmn

moddir = $(libdir)

mod_LTLIBRARIES = libqseawk-dir.la
libqseawk_dir_la_SOURCES = mod-dir.c
libqseawk_dir_la_CPPFLAGS = $(CPPFLAGS_MOD_COMMON)
libqseawk_dir_la_LDFLAGS = $(LDFLAGS_MOD_COMMON)
libqseawk_dir_la_LIBADD = $(LIBADD_MOD_COMMON)

mod_LTLIBRARIES += libqseawk-str.la
libqseawk_str_la_SOURCES = mod-str.c
libqseawk_str_la_CPPFLAGS = $(CPPFLAGS_MOD_COMMON)
libqseawk_str_la_LDFLAGS = $(LDFLAGS_MOD_COMMON)
libqseawk_str_la_LIBADD = $(LIBADD_MOD_COMMON)

mod_LTLIBRARIES += libqseawk-sys.la
libqseawk_sys_la_SOURCES = mod-sys.c
libqseawk_sys_la_CPPFLAGS = $(CPPFLAGS_MOD_COMMON)
libqseawk_sys_la_LDFLAGS = $(LDFLAGS_MOD_COMMON)
libqseawk_sys_la_LIBADD = $(LIBADD_MOD_COMMON)

if HAVE_C_MPI
mod_LTLIBRARIES += libqseawk-mpi.la
libqseawk_mpi_la_SOURCES = mod-mpi.c
libqseawk_mpi_la_CPPFLAGS = $(CPPFLAGS_MOD_COMMON) $(MPI_CFLAGS)
libqseawk_mpi_la_LDFLAGS = $(LDFLAGS_MOD_COMMON) $(MPI_CLDFLAGS)
libqseawk_mpi_la_LIBADD = $(LIBADD_MOD_COMMON)
endif

if HAVE_LIBUCI
mod_LTLIBRARIES += libqseawk-uci.la
libqseawk_uci_la_SOURCES = mod-uci.c
libqseawk_uci_la_CPPFLAGS = $(CPPFLAGS_MOD_COMMON) 
libqseawk_uci_la_LDFLAGS = $(LDFLAGS_MOD_COMMON) 
libqseawk_uci_la_LIBADD = $(LIBADD_MOD_COMMON) $(UCI_LIBS)
endif


