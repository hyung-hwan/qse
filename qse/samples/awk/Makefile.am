AM_CPPFLAGS = -I$(top_srcdir)/include

bin_PROGRAMS = awk01 awk02 awk03 awk04

LDFLAGS = -L../../lib/cmn -L../../lib/awk
LDADD = -lqseawk -lqsecmn $(LIBM)

awk01_SOURCES = awk01.c
awk02_SOURCES = awk02.c
awk03_SOURCES = awk03.c
awk04_SOURCES = awk04.c

if ENABLE_CXX
bin_PROGRAMS += awk05 awk06 awk07 awk08

awk05_SOURCES = awk05.cpp
awk06_SOURCES = awk06.cpp
awk07_SOURCES = awk07.cpp
awk08_SOURCES = awk08.cpp

awk05_LDADD = -lqseawkxx $(LDADD)
awk06_LDADD = -lqseawkxx $(LDADD)
awk07_LDADD = -lqseawkxx $(LDADD)
awk08_LDADD = -lqseawkxx $(LDADD)
endif
