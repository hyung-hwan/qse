AUTOMAKE_OPTIONS = nostdinc

AM_CPPFLAGS = \
	-I$(top_builddir)/include \
	-I$(top_srcdir)/include 

bin_PROGRAMS = qsestx 

qsestx_SOURCES = stx.c
qsestx_LDFLAGS = -L../../lib/stx -L../../lib/cmn
qsestx_LDADD = -lqsestx -lqsecmn $(LIBM)

if WIN32 
qsestx_LDADD += $(UNICOWS_LIBS)
endif
