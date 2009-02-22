AM_CPPFLAGS = -I$(top_srcdir)/include

bin_PROGRAMS = awk01 awk02 awk03

LDFLAGS = -L../../lib/cmn -L../../lib/utl -L../../lib/awk
LDADD = -lqseawk -lqseutl -lqsecmn $(LIBM)

awk01_SOURCES = awk01.c
awk02_SOURCES = awk02.c
awk03_SOURCES = awk03.c
