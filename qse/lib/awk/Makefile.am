AUTOMAKE_OPTIONS = nostdinc

CPPFLAGS_ALL_COMMON = \
	-I$(top_builddir)/include \
	-I$(top_srcdir)/include

LDFLAGS_ALL_COMMON = -L. -L../cmn -version-info 1:0:0 -no-undefined

##################################################
# MAIN LIBRARY 
##################################################

CPPFLAGS_LIB_COMMON = $(CPPFLAGS_ALL_COMMON) $(LTDLINCL)
LDFLAGS_LIB_COMMON = $(LDFLAGS_ALL_COMMON) 
LIBADD_LIB_COMMON = -lqsecmn $(LIBM) $(LIBLTDL) 

if WIN32
# you must adjust the value of DEFAULT_MODPOSTFIX according 
# to the first number in -version-info above
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
libqseawkxx_la_LIBADD = -lqseawk -lqsecmnxx -lqseawk -lqsecmn $(LIBADD_LIB_COMMON) 
endif

if ENABLE_STATIC_MODULE

##################################################
# STATIC MODULES BUILT INTO MAIN LIBRARY
##################################################
libqseawk_la_SOURCES += \
	mod-dir.c mod-dir.h \
	mod-str.c mod-str.h \
	mod-sys.c mod-sys.h

if HAVE_C_MPI
libqseawk_la_SOURCES += mod-mpi.c mod-mpi.h
libqseawk_la_CPPFLAGS += $(MPI_CFLAGS)
libqseawk_la_LDFLAGS += $(MPI_CLDFLAGS)
endif

if HAVE_LIBUCI
libqseawk_la_SOURCES += mod-uci.c mod-uci.h
libqseawk_la_LIBADD += $(UCI_LIBS)
endif

else
##################################################
# DYNAMIC MODULES 
##################################################

CPPFLAGS_MOD_COMMON = $(CPPFLAGS_ALL_COMMON)
LDFLAGS_MOD_COMMON = $(LDFLAGS_ALL_COMMON)
LIBADD_MOD_COMMON = -lqseawk -lqsecmn

modexecdir = $(libdir)

modexec_LTLIBRARIES = libqseawk-dir.la
libqseawk_dir_la_SOURCES = mod-dir.c mod-dir.h
libqseawk_dir_la_CPPFLAGS = $(CPPFLAGS_MOD_COMMON)
libqseawk_dir_la_LDFLAGS = $(LDFLAGS_MOD_COMMON)
libqseawk_dir_la_LIBADD = $(LIBADD_MOD_COMMON)

modexec_LTLIBRARIES += libqseawk-str.la
libqseawk_str_la_SOURCES = mod-str.c mod-str.h
libqseawk_str_la_CPPFLAGS = $(CPPFLAGS_MOD_COMMON)
libqseawk_str_la_LDFLAGS = $(LDFLAGS_MOD_COMMON)
libqseawk_str_la_LIBADD = $(LIBADD_MOD_COMMON)

modexec_LTLIBRARIES += libqseawk-sys.la
libqseawk_sys_la_SOURCES = mod-sys.c mod-sys.h
libqseawk_sys_la_CPPFLAGS = $(CPPFLAGS_MOD_COMMON)
libqseawk_sys_la_LDFLAGS = $(LDFLAGS_MOD_COMMON)
libqseawk_sys_la_LIBADD = $(LIBADD_MOD_COMMON)

if HAVE_C_MPI
modexec_LTLIBRARIES += libqseawk-mpi.la
libqseawk_mpi_la_SOURCES = mod-mpi.c mod-mpi.h
libqseawk_mpi_la_CPPFLAGS = $(CPPFLAGS_MOD_COMMON) $(MPI_CFLAGS)
libqseawk_mpi_la_LDFLAGS = $(LDFLAGS_MOD_COMMON) $(MPI_CLDFLAGS)
libqseawk_mpi_la_LIBADD = $(LIBADD_MOD_COMMON)
endif

if HAVE_LIBUCI
modexec_LTLIBRARIES += libqseawk-uci.la
libqseawk_uci_la_SOURCES = mod-uci.c mod-uci.h
libqseawk_uci_la_CPPFLAGS = $(CPPFLAGS_MOD_COMMON) 
libqseawk_uci_la_LDFLAGS = $(LDFLAGS_MOD_COMMON) 
libqseawk_uci_la_LIBADD = $(LIBADD_MOD_COMMON) $(UCI_LIBS)
endif

endif
