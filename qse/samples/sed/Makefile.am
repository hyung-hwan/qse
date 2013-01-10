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

bin_PROGRAMS = sed01 

sed01_SOURCES = sed01.c 
sed01_LDADD = $(LDADD)

if ENABLE_CXX

CXXLIB = -lqsesedxx -lqsecmnxx
bin_PROGRAMS += sed21 sed22

sed21_SOURCES = sed21.cpp
sed21_LDADD = $(CXXLIB) $(LDADD)

sed22_SOURCES = sed22.cpp
sed22_LDADD = $(CXXLIB) $(LDADD)

endif
