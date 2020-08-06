AiUTOMAKE_OPTIONS = nostdinc

AM_CPPFLAGS = \
	-I$(top_builddir)/include \
	-I$(top_srcdir)/include \
	-I$(includedir) 
	
bin_PROGRAMS = skvenv01 

AM_LDFLAGS = -L../../lib/cmn -L../../lib/si -L../../lib/xli 
LDADD = -lqsexli -lqsesi -lqsecmn 

if WIN32 
if WCHAR
LDADD += $(UNICOWS_LIBS)
endif
endif


if ENABLE_CXX

skvenv01_SOURCES = skvenv01.cpp

endif
