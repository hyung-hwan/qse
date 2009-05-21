AM_CPPFLAGS = -I$(top_srcdir)/include

bin_PROGRAMS = sed01 sed02

LDFLAGS = -L../../lib/cmn -L../../lib/utl -L../../lib/sed
LDADD = -lqsesed -lqseutl -lqsecmn $(LIBM)

sed01_SOURCES = sed01.c

sed02_SOURCES = sed02.cpp
sed02_LDADD = -lqsesed++ $(LDADD)
