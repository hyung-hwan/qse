
AUTOMAKE_OPTIONS = nostdinc

AM_CPPFLAGS = -I$(top_srcdir)/include

lib_LTLIBRARIES = libqsescm.la 

libqsescm_la_SOURCES = scm.h scm.c err.c 
libqsescm_la_LDFLAGS = -L../cmn -version-info 1:0:0 -no-undefined
libqsescm_la_LIBADD = -lqsecmn
