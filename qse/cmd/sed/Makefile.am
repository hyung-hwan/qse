AUTOMAKE_OPTIONS = nostdinc

AM_CPPFLAGS = \
	-I$(top_builddir)/include \
	-I$(top_srcdir)/include 

bin_PROGRAMS = qsesed

qsesed_SOURCES = sed.c
qsesed_LDFLAGS = -L../../lib/sed -L../../lib/cmn 
qsesed_LDADD = -lqsesed -lqsecmn

if WIN32 
if WCHAR
qsesed_LDADD += $(UNICOWS_LIBS)
endif
endif
