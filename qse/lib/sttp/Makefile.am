AUTOMAKE_OPTIONS = nostdinc

AM_CPPFLAGS = \
	-I$(top_builddir)/include \
	-I$(top_srcdir)/include

#noinst_HEADERS =

lib_LTLIBRARIES =

if ENABLE_CXX
lib_LTLIBRARIES += libqsesttpxx.la
libqsesttpxx_la_SOURCES = Sttp.cpp
libqsesttpxx_la_LDFLAGS = -L. -L../cmn -L../si -version-info 1:0:0 -no-undefined
libqsesttpxx_la_LIBADD = -lqsecmnxx -lqsesi -lqsecmn
endif

