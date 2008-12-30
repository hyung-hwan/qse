
AUTOMAKE_OPTIONS = nostdinc

AM_CPPFLAGS = -I$(top_srcdir)/include

lib_LTLIBRARIES = libqseawk.la
libqseawk_la_SOURCES = awk.c err.c tree.c parse.c run.c rec.c val.c func.c misc.c extio.c std.c awk.h extio.h func.h misc.h parse.h run.h tree.h 
libqseawk_la_LDFLAGS= -L../cmn -L../utl -version-info 1:0:0 -no-undefined
libqseawk_la_LIBADD= -lqsecmn -lqseutl

if ENABLE_CXX
lib_LTLIBRARIES += libqseawk++.la
libqseawk___la_SOURCES = Awk.cpp StdAwk.cpp
libqseawk___la_LDFLAGS = -L. -L../cmn -L../utl -version-info 1:0:0 -no-undefined
libqseawk___la_LIBADD = -lqseawk  -lqsecmn -lqseutl
endif
