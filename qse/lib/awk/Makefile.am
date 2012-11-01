AUTOMAKE_OPTIONS = nostdinc

AM_CPPFLAGS = \
	-I$(top_builddir)/include \
	-I$(top_srcdir)/include \
	-I$(includedir) \
	-DDEFAULT_MODDIR=\"$(libdir)/qse\" $(LTDLINCL)

lib_LTLIBRARIES = libqseawk.la
libqseawk_la_SOURCES = awk.c err.c tree.c parse.c run.c rec.c val.c fnc.c misc.c rio.c std.c awk.h err.h rio.h val.h fnc.h misc.h parse.h run.h tree.h 
libqseawk_la_LDFLAGS = -L../cmn -L$(libdir) -version-info 1:0:0 -no-undefined
libqseawk_la_LIBADD = -lqsecmn $(LIBM) $(LIBLTDL)

if ENABLE_CXX
lib_LTLIBRARIES += libqseawkxx.la
libqseawkxx_la_SOURCES = Awk.cpp StdAwk.cpp
libqseawkxx_la_LDFLAGS = -L. -L../cmn -L$(libdir) -version-info 1:0:0 -no-undefined
libqseawkxx_la_LIBADD = -lqseawk  -lqsecmn $(LIBM) $(LIBLTDL)
endif

