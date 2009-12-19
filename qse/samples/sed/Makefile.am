AM_CPPFLAGS = -I$(top_srcdir)/include

bin_PROGRAMS = 

LDFLAGS = -L../../lib/cmn -L../../lib/sed
LDADD = -lqsesed -lqsecmn $(LIBM)

if ENABLE_CXX

CXXLIB = -lqsesedxx -lqsecmnxx

bin_PROGRAMS += sed02 sed03 

sed02_SOURCES = sed02.cpp
sed02_LDADD = $(CXXLIB) $(LDADD)

sed03_SOURCES = sed03.cpp
sed03_LDADD = $(CXXLIB) $(LDADD)
endif
