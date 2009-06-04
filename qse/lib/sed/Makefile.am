
AM_CPPFLAGS = -I$(top_builddir)/include

lib_LTLIBRARIES = libqsesed.la
libqsesed_la_SOURCES = sed.c err.c sed.h
libqsesed_la_LDFLAGS = -version-info 1:0:0 -no-undefined -L../cmn 
libqsesed_la_LIBADD = -lqsecmn 

if ENABLE_CXX
lib_LTLIBRARIES += libqsesed++.la
libqsesed___la_SOURCES = Sed.cpp StdSed.cpp
libqsesed___la_LDFLAGS = -L. -L../cmn -version-info 1:0:0 -no-undefined
libqsesed___la_LIBADD = -lqsesed  -lqsecmn 
endif

