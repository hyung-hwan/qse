
AM_CFLAGS = -I$(top_builddir)/include

lib_LTLIBRARIES = libqseutl.la
libqseutl_la_SOURCES = \
	assert.c http.c main.c stdio.c \
	sed.c tgp.c \
	sed.h
libqseutl_la_LDFLAGS = -version-info 1:0:0 -no-undefined -L../cmn
libqseutl_la_LIBADD = -lqsecmn
