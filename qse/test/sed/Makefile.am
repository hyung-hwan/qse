AM_CPPFLAGS = -I$(top_srcdir)/include

bin_PROGRAMS = sed01

LDFLAGS = -L../../lib/cmn -L../../lib/utl -L../../lib/sed
LDADD = -lqsesed -lqseutl -lqsecmn $(LIBM)

sed01_SOURCES = sed01.c

if ENABLE_CXX
bin_PROGRAMS += sed02 sed03 

sed02_SOURCES = sed02.cpp
sed02_LDADD = -lqsesed++ $(LDADD)

sed03_SOURCES = sed03.cpp
sed03_LDADD = -lqsesed++ $(LDADD)
endif
