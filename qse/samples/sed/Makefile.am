AUTOMAKE_OPTIONS = nostdinc

AM_CPPFLAGS = \
	-I$(top_builddir)/include \
	-I$(top_srcdir)/include \
	-I$(includedir)


LDFLAGS = -L../../lib/cmn -L../../lib/sed
LDADD = -lqsesed -lqsecmn 

if WIN32 
if WCHAR
LDADD += $(UNICOWS_LIBS)
endif
endif

CMNFILES = sed00.c sed00.h
bin_PROGRAMS = sed01 sed02 sed03

sed01_SOURCES = sed01.c $(CMNFILES)
sed01_LDADD = $(LDADD)

sed02_SOURCES = sed02.c $(CMNFILES)
sed02_LDADD = $(LDADD)

sed03_SOURCES = sed03.c $(CMNFILES)
sed03_LDADD = $(LDADD)

if ENABLE_CXX

CXXLIB = -lqsesedxx -lqsecmnxx
bin_PROGRAMS += sed21 sed22

sed21_SOURCES = sed21.cpp $(CMNFILES)
sed21_LDADD = $(CXXLIB) $(LDADD)

sed22_SOURCES = sed22.cpp $(CMNFILES)
sed22_LDADD = $(CXXLIB) $(LDADD)

endif
