AUTOMAKE_OPTIONS = nostdinc

##AM_CFLAGS = $(PTHREAD_CFLAGS)

CPPFLAGS_ALL_COMMON = \
	-I$(top_builddir)/include \
	-I$(top_srcdir)/include

if ENABLE_STATIC_MODULE
LDFLAGS_ALL_COMMON = -L../awkmod -L../cmn -L../si 
else
LDFLAGS_ALL_COMMON = -L. -L../cmn -L../si 
endif

##################################################
# MAIN LIBRARY 
##################################################

CPPFLAGS_LIB_COMMON = $(CPPFLAGS_ALL_COMMON) $(LTDLINCL)
LDFLAGS_LIB_COMMON = $(LDFLAGS_ALL_COMMON) -version-info 1:0:0 -no-undefined
LIBADD_LIB_COMMON = -lqsecmn -lqsesi $(LIBM)

if ENABLE_LIBLTDL
LIBADD_LIB_COMMON += $(LTDL_LIBS)
else
LIBADD_LIB_COMMON += $(DL_LIBS)
endif

if WIN32
# you must adjust the value of DEFAULT_MODPOSTFIX according 
# to the first number in -version-info above
CPPFLAGS_LIB_COMMON += -DQSE_AWK_DEFAULT_MODPREFIX=\"libqseawk-\" -DQSE_AWK_DEFAULT_MODPOSTFIX=\"-1\"
else
CPPFLAGS_LIB_COMMON += -DQSE_AWK_DEFAULT_MODPREFIX=\"$(libdir)/libqseawk-\" -DQSE_AWK_DEFAULT_MODPOSTFIX=\"\"
endif

noinst_HEADERS = awk-prv.h err.h rio.h val.h fnc.h misc.h parse.h run.h tree.h std.h

lib_LTLIBRARIES = libqseawk.la
libqseawk_la_SOURCES = awk.c err.c tree.c parse.c run.c rec.c val.c val-imp.h fnc.c misc.c misc-imp.h rio.c std.c
libqseawk_la_CPPFLAGS = $(CPPFLAGS_LIB_COMMON)
libqseawk_la_LDFLAGS = $(LDFLAGS_LIB_COMMON)
libqseawk_la_LIBADD = $(LIBADD_LIB_COMMON)
libqseawk_la_DEPENDENCIES =

if ENABLE_CXX
lib_LTLIBRARIES += libqseawkxx.la
libqseawkxx_la_SOURCES = Awk.cpp StdAwk.cpp
libqseawkxx_la_CPPFLAGS = $(CPPFLAGS_LIB_COMMON)
libqseawkxx_la_LDFLAGS = $(LDFLAGS_LIB_COMMON)
libqseawkxx_la_LIBADD = -lqseawk -lqsecmnxx -lqseawk -lqsesi -lqsecmn $(LIBADD_LIB_COMMON) 
libqseawkxx_la_DEPENDENCIES =
endif

if ENABLE_STATIC_MODULE

##################################################
# STATIC MODULES BUILT INTO MAIN LIBRARY
##################################################
libqseawk_la_SOURCES += \
	mod-dir.c mod-dir.h \
	mod-math.c mod-math.h \
	mod-str.c mod-str.h \
	mod-sys.c mod-sys.h
##libqseawk_la_LIBADD +=

if ENABLE_AWKMOD_MPI
libqseawk_la_LIBADD += -lqseawk-mpi
libqseawk_la_DEPENDENCIES += ../awkmod/libqseawk-mpi.la
endif

if ENABLE_AWKMOD_MYSQL
libqseawk_la_LIBADD += -lqseawk-mysql
libqseawk_la_DEPENDENCIES += ../awkmod/libqseawk-mysql.la
endif

if ENABLE_AWKMOD_SED
libqseawk_la_LIBADD += -lqseawk-sed
libqseawk_la_DEPENDENCIES += ../awkmod/libqseawk-sed.la
endif

if ENABLE_AWKMOD_UCI
libqseawk_la_LIBADD += -lqseawk-uci
libqseawk_la_DEPENDENCIES += ../awkmod/libqseawk-uci.la
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
libqseawk_dir_la_LDFLAGS = $(LDFLAGS_MOD_COMMON) -L../si
libqseawk_dir_la_LIBADD = $(LIBADD_MOD_COMMON) -lqsesi

modexec_LTLIBRARIES += libqseawk-math.la
libqseawk_math_la_SOURCES = mod-math.c mod-math.h
libqseawk_math_la_CPPFLAGS = $(CPPFLAGS_MOD_COMMON)
libqseawk_math_la_LDFLAGS = $(LDFLAGS_MOD_COMMON)
libqseawk_math_la_LIBADD = $(LIBADD_MOD_COMMON)

modexec_LTLIBRARIES += libqseawk-str.la
libqseawk_str_la_SOURCES = mod-str.c mod-str.h
libqseawk_str_la_CPPFLAGS = $(CPPFLAGS_MOD_COMMON)
libqseawk_str_la_LDFLAGS = $(LDFLAGS_MOD_COMMON)
libqseawk_str_la_LIBADD = $(LIBADD_MOD_COMMON)

modexec_LTLIBRARIES += libqseawk-sys.la
libqseawk_sys_la_SOURCES = mod-sys.c mod-sys.h
libqseawk_sys_la_CPPFLAGS = $(CPPFLAGS_MOD_COMMON)
libqseawk_sys_la_LDFLAGS = $(LDFLAGS_MOD_COMMON) -L../si
libqseawk_sys_la_LIBADD = $(LIBADD_MOD_COMMON) -lqsesi

endif
