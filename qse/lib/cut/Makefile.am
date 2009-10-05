
AM_CPPFLAGS = -I$(top_srcdir)/include

lib_LTLIBRARIES = libqsecut.la
libqsecut_la_SOURCES = cut.c err.c cut.h
libqsecut_la_LDFLAGS = -version-info 1:0:0 -no-undefined -L../cmn 
libqsecut_la_LIBADD = -lqsecmn 

#if ENABLE_CXX
#lib_LTLIBRARIES += libqsecutxx.la
#libqsecutxx_la_SOURCES = Sed.cpp StdSed.cpp
#libqsecutxx_la_LDFLAGS = -L. -L../cmn -version-info 1:0:0 -no-undefined
#libqsecutxx_la_LIBADD = -lqsecut  -lqsecmn 
#endif

