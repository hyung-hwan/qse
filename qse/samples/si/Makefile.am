AUTOMAKE_OPTIONS = nostdinc

AM_CPPFLAGS = \
	-I$(top_builddir)/include \
	-I$(top_srcdir)/include \
	-I$(includedir)

bin_PROGRAMS = \
	fio01 \
	fio02 \
	pio01 \
	rwl01 \
	sio01 \
	sio02 \
	sio03 
	
AM_LDFLAGS = -L../../lib/si  -L../../lib/cmn
AM_CFLAGS = $(PTHREAD_CFLAGS)
LDADD = -lqsesi -lqsecmn $(PTHREAD_LIBS)

if WIN32 
if WCHAR
LDADD += $(UNICOWS_LIBS)
endif
endif

fio01_SOURCES = fio01.c
fio02_SOURCES = fio02.c
pio01_SOURCES = pio01.c
rwl01_SOURCES = rwl01.c
sio01_SOURCES = sio01.c
sio02_SOURCES = sio02.c
sio03_SOURCES = sio03.c

