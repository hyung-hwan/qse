AUTOMAKE_OPTIONS = nostdinc

AM_CPPFLAGS = \
	-I$(top_builddir)/include \
	-I$(top_srcdir)/include \
	-I$(includedir)


LDFLAGS = -L../../lib/cmn -L../../lib/sed
LDADD = -lqsesed -lqsecmn 

if WIN32 
LDADD += $(UNICOWS_LIBS)
endif

bin_PROGRAMS = sed01

sed01_SOURCES = sed01.c
sed01_LDADD = $(LDADD)

if ENABLE_CXX

CXXLIB = -lqsesedxx -lqsecmnxx
bin_PROGRAMS += sed02 sed03 

sed02_SOURCES = sed02.cpp
sed02_LDADD = $(CXXLIB) $(LDADD)

sed03_SOURCES = sed03.cpp
sed03_LDADD = $(CXXLIB) $(LDADD)

endif
