AUTOMAKE_OPTIONS = nostdinc

AM_CPPFLAGS = \
	-I$(top_builddir)/include \
	-I$(top_srcdir)/include \
	-I$(includedir)

LDFLAGS = -L../../lib/cmn -L../../lib/cut
LDADD = -lqsecut -lqsecmn $(LIBM)

if WIN32 
LDADD += $(UNICOWS_LIBS)
endif

bin_PROGRAMS = cut01

cut01_SOURCES = cut01.c
cut01_LDADD = $(LDADD)

#if ENABLE_CXX
#
#CXXLIB = -lqsecutxx -lqsecmnxx
#bin_PROGRAMS += cut02 cut03 
#
#cut02_SOURCES = cut02.cpp
#cut02_LDADD = $(CXXLIB) $(LDADD)
#
#
#endif
