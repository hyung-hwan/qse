AUTOMAKE_OPTIONS = nostdinc

AM_CPPFLAGS = \
	-I$(top_builddir)/include \
	-I$(top_srcdir)/include

lib_LTLIBRARIES = libqsesys.la
libqsesys_la_SOURCES =  \
	cnd.c \
	intr.c \
	mtx.c \
	rwl.c \
	thr.c \
	thr.h
libqsesys_la_LDFLAGS = -L../cmn -version-info 1:0:0 -no-undefined
libqsesys_la_LIBADD = -lqsecmn 

if ENABLE_CXX
lib_LTLIBRARIES += libqsesysxx.la
libqsesysxx_la_SOURCES = \
	SocketAddress.cpp
libqsesysxx_la_LDFLAGS = -L. -L../cmn -version-info 1:0:0 -no-undefined
libqsesysxx_la_LIBADD = -lqsecmnxx -lqsecmn 
endif

