
AUTOMAKE_OPTIONS = nostdinc

AM_CPPFLAGS = -I$(top_srcdir)/include

lib_LTLIBRARIES = libqsetgp.la 

libqsetgp_la_SOURCES = tgp.c 
libqsetgp_la_LDFLAGS = -L../cmn -version-info 1:0:0 -no-undefined
libqsetgp_la_LIBADD = -lqsecmn
