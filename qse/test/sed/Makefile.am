AM_CPPFLAGS = -I$(top_srcdir)/include

bin_PROGRAMS = sed01 

LDFLAGS = -L../../lib/cmn -L../../lib/utl -L../../lib/sed
LDADD = -lqsesed -lqseutl -lqsecmn $(LIBM)

sed01_SOURCES = sed01.c
