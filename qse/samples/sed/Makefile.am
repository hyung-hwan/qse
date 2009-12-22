AM_CPPFLAGS = -I$(top_srcdir)/include

bin_PROGRAMS = sed01

LDFLAGS = -L../../lib/cmn -L../../lib/sed
LDADD = -lqsesed -lqsecmn $(LIBM)

if ENABLE_CXX

CXXLIB = -lqsesedxx -lqsecmnxx

sed01_SOURCES = sed01.c
sed01_LDADD = $(LDADD)

bin_PROGRAMS += sed02 sed03 

sed02_SOURCES = sed02.cpp
sed02_LDADD = $(CXXLIB) $(LDADD)

sed03_SOURCES = sed03.cpp
sed03_LDADD = $(CXXLIB) $(LDADD)
endif
