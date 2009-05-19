
AM_CFLAGS = -I$(top_builddir)/include

lib_LTLIBRARIES = libqsesed.la
libqsesed_la_SOURCES = sed.c sed.h
libqsesed_la_LDFLAGS = -version-info 1:0:0 -no-undefined -L../cmn -L../utl
libqsesed_la_LIBADD = -lqsecmn -lqseutl
