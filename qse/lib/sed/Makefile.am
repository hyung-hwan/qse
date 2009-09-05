
AM_CPPFLAGS = -I$(top_srcdir)/include

lib_LTLIBRARIES = libqsesed.la
libqsesed_la_SOURCES = sed.c err.c sed.h
libqsesed_la_LDFLAGS = -version-info 1:0:0 -no-undefined -L../cmn 
libqsesed_la_LIBADD = -lqsecmn 

if ENABLE_CXX
lib_LTLIBRARIES += libqsesedxx.la
libqsesedxx_la_SOURCES = Sed.cpp StdSed.cpp
libqsesedxx_la_LDFLAGS = -L. -L../cmn -version-info 1:0:0 -no-undefined
libqsesedxx_la_LIBADD = -lqsesed  -lqsecmn 
endif

