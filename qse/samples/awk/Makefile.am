AUTOMAKE_OPTIONS = nostdinc

AM_CPPFLAGS = \
	-I$(top_builddir)/include \
	-I$(top_srcdir)/include \
	-I$(includedir)

bin_PROGRAMS = awk01 awk02 awk03 awk04 awk09 awk10

LDFLAGS = -L../../lib/awk -L../../lib/cmn
LDADD = -lqseawk -lqsecmn $(LIBM)

awk01_SOURCES = awk01.c
awk02_SOURCES = awk02.c
awk03_SOURCES = awk03.c
awk04_SOURCES = awk04.c
awk09_SOURCES = awk09.c
awk10_SOURCES = awk10.c

if ENABLE_CXX

CXXLIB = -lqseawkxx -lqsecmnxx

bin_PROGRAMS += awk05 awk06 awk07 awk08

awk05_SOURCES = awk05.cpp
awk06_SOURCES = awk06.cpp
awk07_SOURCES = awk07.cpp
awk08_SOURCES = awk08.cpp

awk05_LDADD = $(CXXLIB) $(LDADD)
awk06_LDADD = $(CXXLIB) $(LDADD)
awk07_LDADD = $(CXXLIB) $(LDADD)
awk08_LDADD = $(CXXLIB) $(LDADD)
endif
