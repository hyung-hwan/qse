AUTOMAKE_OPTIONS = nostdinc

AM_CPPFLAGS = \
	-I$(top_builddir)/include \
	-I$(top_srcdir)/include \
	-I$(includedir)

lib_LTLIBRARIES = libqsecut.la
libqsecut_la_SOURCES = cut.c err.c cut.h std.c
libqsecut_la_LDFLAGS = -L../cmn  -L$(libdir) -version-info 1:0:0 -no-undefined
libqsecut_la_LIBADD = -lqsecmn 

if ENABLE_CXX
lib_LTLIBRARIES += libqsecutxx.la
libqsecutxx_la_SOURCES = Cut.cpp StdCut.cpp
libqsecutxx_la_LDFLAGS = -L. -L../cmn -L$(libdir) -version-info 1:0:0 -no-undefined
libqsecutxx_la_LIBADD = -lqsecut  -lqsecmn 
endif

