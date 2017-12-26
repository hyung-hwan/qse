AiUTOMAKE_OPTIONS = nostdinc

AM_CPPFLAGS = \
	-I$(top_builddir)/include \
	-I$(top_srcdir)/include \
	-I$(includedir) 
	
bin_PROGRAMS = raddic01 

AM_LDFLAGS = -L../../lib/cmn -L../../lib/si -L../../lib/rad 
LDADD = -lqserad -lqsesi -lqsecmn 

if WIN32 
if WCHAR
LDADD += $(UNICOWS_LIBS)
endif
endif

raddic01_SOURCES = raddic01.c

raddic01_LDADD = $(LDADD)
