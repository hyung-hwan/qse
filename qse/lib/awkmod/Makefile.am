AUTOMAKE_OPTIONS = nostdinc

##AM_CFLAGS = $(PTHREAD_CFLAGS)

CPPFLAGS_COMMON = \
	-I$(top_builddir)/include \
	-I$(top_srcdir)/include \
	-I$(includedir)

##################################################
if ENABLE_STATIC_MODULE
##################################################

#-------------------------------------------------
# STATIC MODULES BUILT INTO MAIN LIBRARY
#-------------------------------------------------
LDFLAGS_COMMON =-L$(libdir) -version-info 1:0:0 -no-undefined
LIBADD_COMMON =

noinst_LTLIBRARIES =

if ENABLE_AWKMOD_MPI
noinst_LTLIBRARIES += libqseawk-mpi.la
endif

if ENABLE_AWKMOD_MYSQL
noinst_LTLIBRARIES += libqseawk-mysql.la
endif

if ENABLE_AWKMOD_SED
noinst_LTLIBRARIES += libqseawk-sed.la
endif

if ENABLE_AWKMOD_UCI
noinst_LTLIBRARIES += libqseawk-uci.la
endif

##################################################
else
##################################################

#-------------------------------------------------
# DYNAMIC MODULES
#-------------------------------------------------
LDFLAGS_COMMON = -L../awk -L../si -L../cmn -L$(libdir) -version-info 1:0:0 -no-undefined
LIBADD_COMMON = -lqseawk -lqsesi -lqsecmn

pkgmodexecdir = $(libdir)
pkgmodexec_LTLIBRARIES =

if ENABLE_AWKMOD_MPI
pkgmodexec_LTLIBRARIES += libqseawk-mpi.la
endif

if ENABLE_AWKMOD_MYSQL
pkgmodexec_LTLIBRARIES += libqseawk-mysql.la
endif

if ENABLE_AWKMOD_SED
pkgmodexec_LTLIBRARIES += libqseawk-sed.la
endif

if ENABLE_AWKMOD_UCI
pkgmodexec_LTLIBRARIES += libqseawk-uci.la
endif

##################################################
endif
##################################################


#-------------------------------------------------
# ACTUAL MODULES
#-------------------------------------------------
if ENABLE_AWKMOD_MPI
libqseawk_mpi_la_SOURCES = mod-mpi.c mod-mpi.h
libqseawk_mpi_la_CPPFLAGS = $(CPPFLAGS_COMMON) $(MPI_CFLAGS)
libqseawk_mpi_la_LDFLAGS = $(LDFLAGS_COMMON) $(MPI_CLDFLAGS)
libqseawk_mpi_la_LIBADD = $(LIBADD_COMMON)
endif

if ENABLE_AWKMOD_MYSQL
libqseawk_mysql_la_SOURCES = mod-mysql.c mod-mysql.h
libqseawk_mysql_la_CPPFLAGS = $(CPPFLAGS_COMMON) $(MYSQL_CFLAGS)
libqseawk_mysql_la_LDFLAGS = $(LDFLAGS_COMMON) $(MYSQL_LDFLAGS)
libqseawk_mysql_la_LIBADD = $(LIBADD_COMMON) 
endif

if ENABLE_AWKMOD_SED
libqseawk_sed_la_SOURCES = mod-sed.c mod-sed.h
libqseawk_sed_la_CPPFLAGS = $(CPPFLAGS_COMMON)
## ../sed must be placed before LDFLAGS_COMMON because it should precede $(libdir)
libqseawk_sed_la_LDFLAGS = -L../sed $(LDFLAGS_COMMON)
libqseawk_sed_la_LIBADD = $(LIBADD_COMMON) -lqsesed
endif

if ENABLE_AWKMOD_UCI
libqseawk_uci_la_SOURCES = mod-uci.c mod-uci.h
libqseawk_uci_la_CPPFLAGS = $(CPPFLAGS_COMMON)
libqseawk_uci_la_LDFLAGS = $(LDFLAGS_COMMON)
libqseawk_uci_la_LIBADD = $(LIBADD_COMMON) $(UCI_LIBS)
endif
