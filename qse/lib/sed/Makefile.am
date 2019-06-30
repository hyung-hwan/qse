AUTOMAKE_OPTIONS = nostdinc

AM_CPPFLAGS = \
	-I$(top_builddir)/include \
	-I$(top_srcdir)/include

noinst_HEADERS = sed-prv.h

lib_LTLIBRARIES = libqsesed.la
libqsesed_la_SOURCES = sed.c err.c std.c
libqsesed_la_LDFLAGS = -L../cmn -L../si -version-info 1:0:0 -no-undefined
libqsesed_la_LIBADD = -lqsecmn  -lqsesi
libqsesed_la_DEPENDENCIES =  ../../lib/cmn/libqsecmn.la ../../lib/si/libqsesi.la

if ENABLE_CXX
lib_LTLIBRARIES += libqsesedxx.la
libqsesedxx_la_SOURCES = Sed.cpp StdSed.cpp
libqsesedxx_la_LDFLAGS = -L. -L../cmn -L../si -version-info 1:0:0 -no-undefined
libqsesedxx_la_LIBADD = -lqsesed -lqsecmnxx -lqsesi -lqsecmn 
libqsesedxx_la_DEPENDENCIES = libqsesed.la ../../lib/cmn/libqsecmnxx.la ../../lib/si/libqsesi.la ../../lib/cmn/libqsecmn.la
endif

