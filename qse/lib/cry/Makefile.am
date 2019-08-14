AUTOMAKE_OPTIONS = nostdinc

AM_CPPFLAGS = \
	-I$(top_builddir)/include \
	-I$(top_srcdir)/include

#noinst_HEADERS =

lib_LTLIBRARIES = libqsecry.la
libqsecry_la_SOURCES =  \
	blowfish.c  \
	kseed.c \
	md5.c \
	sha1.c \
	sha2.c

libqsecry_la_CFLAGS = 
libqsecry_la_LDFLAGS = -L../cmn -version-info 1:0:0 -no-undefined
libqsecry_la_LIBADD = -lqsecmn 
libqsecry_la_DEPENDENCIES = ../../lib/cmn/libqsecmn.la
