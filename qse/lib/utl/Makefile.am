
AM_CFLAGS = -I$(top_builddir)/include

lib_LTLIBRARIES = libqseutl.la
libqseutl_la_SOURCES = \
	assert.c http.c main.c stdio.c \
	tgp.c \
	tgp.h
libqseutl_la_LDFLAGS = -version-info 1:0:0 -no-undefined -L../cmn
libqseutl_la_LIBADD = -lqsecmn


if ENABLE_CXX
lib_LTLIBRARIES += libqsesed++.la
libqsesed___la_SOURCES = Sed.cpp
libqsesed___la_LDFLAGS = -L. -L../cmn -L../utl -version-info 1:0:0 -no-undefined
libqsesed___la_LIBADD = -lqsesed  -lqsecmn -lqseutl
endif

