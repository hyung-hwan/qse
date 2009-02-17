AM_CPPFLAGS = -I$(top_srcdir)/include

bin_PROGRAMS = awk01

LDFLAGS = -L../../lib/cmn -L../../lib/utl -L../../lib/awk
LDADD = -lqseawk -lqseutl -lqsecmn $(LIBM)

awk01_SOURCES = awk01.c
