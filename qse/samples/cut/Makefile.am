AM_CPPFLAGS = -I$(top_srcdir)/include

LDFLAGS = -L../../lib/cmn -L../../lib/cut
LDADD = -lqsecut -lqsecmn $(LIBM)

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
