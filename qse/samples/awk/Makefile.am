AUTOMAKE_OPTIONS = nostdinc

AM_CPPFLAGS = \
	-I$(top_builddir)/include \
	-I$(top_srcdir)/include \
	-I$(includedir)

bin_PROGRAMS = awk01 awk02 awk03 awk04 awk05 awk06 awk07 awk08 awk09 awk10 awk11 awk12 awk15

LDFLAGS = -L../../lib/awk -L../../lib/cmn
LDADD = -lqseawk -lqsecmn $(LIBM)

if WIN32 
if WCHAR
LDADD += $(UNICOWS_LIBS)
endif
endif

CMNFILES = awk00.c awk00.h
awk01_SOURCES = awk01.c 
awk02_SOURCES = awk02.c $(CMNFILES)
awk03_SOURCES = awk03.c $(CMNFILES) 
awk04_SOURCES = awk04.c $(CMNFILES)
awk05_SOURCES = awk05.c $(CMNFILES)
awk06_SOURCES = awk06.c $(CMNFILES)
awk07_SOURCES = awk07.c $(CMNFILES)
awk08_SOURCES = awk08.c $(CMNFILES)
awk09_SOURCES = awk09.c $(CMNFILES)
awk10_SOURCES = awk10.c $(CMNFILES)
awk11_SOURCES = awk11.c $(CMNFILES)
awk12_SOURCES = awk12.c $(CMNFILES)
awk15_SOURCES = awk15.c $(CMNFILES)

if ENABLE_CXX

CXXLIB = -lqseawkxx -lqsecmnxx

bin_PROGRAMS += awk21 awk22 awk23 awk24 awk25 awk26 awk27 awk28

awk21_SOURCES = awk21.cpp
awk22_SOURCES = awk22.cpp
awk23_SOURCES = awk23.cpp
awk24_SOURCES = awk24.cpp
awk25_SOURCES = awk25.cpp
awk26_SOURCES = awk26.cpp
awk27_SOURCES = awk27.cpp
awk28_SOURCES = awk27.cpp

awk21_LDADD = $(CXXLIB) $(LDADD)
awk22_LDADD = $(CXXLIB) $(LDADD)
awk23_LDADD = $(CXXLIB) $(LDADD)
awk24_LDADD = $(CXXLIB) $(LDADD)
awk25_LDADD = $(CXXLIB) $(LDADD)
awk26_LDADD = $(CXXLIB) $(LDADD)
awk27_LDADD = $(CXXLIB) $(LDADD)
awk28_LDADD = $(CXXLIB) $(LDADD)
endif
