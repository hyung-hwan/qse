AUTOMAKE_OPTIONS = nostdinc

AM_CPPFLAGS = \
	-I$(top_builddir)/include \
	-I$(top_srcdir)/include \
	-I$(includedir)


bin_PROGRAMS = \
	bf01 \
	sha01
	

AM_CFLAGS = $(PTHREAD_CFLAGS)
AM_LDFLAGS = -L../../lib/cry  -L../../lib/si -L../../lib/cmn
LDADD = -lqsecry -lqsesi -lqsecmn $(LIBM) $(PTHREAD_LIBS)
if WIN32 
if WCHAR
LDADD += $(UNICOWS_LIBS)
endif
endif

bf01_SOURCES = bf01.c
sha01_SOURCES = sha01.c
