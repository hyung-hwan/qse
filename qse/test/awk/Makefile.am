AM_CPPFLAGS = -I$(top_srcdir)/include

bin_PROGRAMS = awk01 awk02 awk03 awk04

LDFLAGS = -L../../lib/cmn -L../../lib/awk
LDADD = -lqseawk -lqsecmn $(LIBM)

awk01_SOURCES = awk01.c
awk02_SOURCES = awk02.c
awk03_SOURCES = awk03.c
awk04_SOURCES = awk04.c

if ENABLE_CXX
bin_PROGRAMS += awk05

awk05_SOURCES = awk05.cpp
awk05_LDADD = -lqseawk++ $(LDADD)
endif
