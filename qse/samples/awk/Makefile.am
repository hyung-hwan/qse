AUTOMAKE_OPTIONS = nostdinc

AM_CPPFLAGS = \
	-I$(top_builddir)/include \
	-I$(top_srcdir)/include \
	-I$(includedir)

bin_PROGRAMS = awk01 awk02 awk03 awk04 awk09 awk10 awk11 awk15

LDFLAGS = -L../../lib/awk -L../../lib/cmn
LDADD = -lqseawk -lqsecmn $(LIBM)

if WIN32 
if WCHAR
LDADD += $(UNICOWS_LIBS)
endif
endif

awk01_SOURCES = awk01.c
awk02_SOURCES = awk02.c
awk03_SOURCES = awk03.c
awk04_SOURCES = awk04.c
awk09_SOURCES = awk09.c
awk10_SOURCES = awk10.c
awk11_SOURCES = awk11.c
awk15_SOURCES = awk15.c

if ENABLE_CXX

CXXLIB = -lqseawkxx -lqsecmnxx

bin_PROGRAMS += awk05 awk06 awk07 awk08 awk12 awk13 awk14

awk05_SOURCES = awk05.cpp
awk06_SOURCES = awk06.cpp
awk07_SOURCES = awk07.cpp
awk08_SOURCES = awk08.cpp
awk12_SOURCES = awk12.cpp
awk13_SOURCES = awk13.cpp
awk14_SOURCES = awk14.cpp

awk05_LDADD = $(CXXLIB) $(LDADD)
awk06_LDADD = $(CXXLIB) $(LDADD)
awk07_LDADD = $(CXXLIB) $(LDADD)
awk08_LDADD = $(CXXLIB) $(LDADD)
awk12_LDADD = $(CXXLIB) $(LDADD)
awk13_LDADD = $(CXXLIB) $(LDADD)
awk14_LDADD = $(CXXLIB) $(LDADD)
endif
