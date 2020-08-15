AiUTOMAKE_OPTIONS = nostdinc

AM_CPPFLAGS = \
	-I$(top_builddir)/include \
	-I$(top_srcdir)/include \
	-I$(includedir) 
	
bin_PROGRAMS = dhcp01 

AM_LDFLAGS = -L../../lib/cmn -L../../lib/si -L../../lib/dhcp 
LDADD = -lqsedhcp -lqsesi -lqsecmn 

if WIN32 
LDADD += $(UNICOWS_LIBS)
endif

dhcp01_SOURCES = dhcp01.c

dhcp01_LDADD = $(LDADD)
