AUTOMAKE_OPTIONS = nostdinc

AM_CPPFLAGS = \
	-I$(top_builddir)/include \
	-I$(top_srcdir)/include \
	-I$(includedir)

bin_PROGRAMS = \
	dir01 \
	fio01 \
	fio02 \
	fs01 \
	fs02 \
	fs03 \
	glob01 \
	log01 \
	nwad01 \
	nwif01 \
	nwif02 \
	pio01 \
	rwl01 \
	sck01 \
	sio01 \
	sio02 \
	sio03  \
	spl01 \
	task01 \
	thr01
	
AM_LDFLAGS = -L../../lib/si  -L../../lib/cmn
AM_CFLAGS = $(PTHREAD_CFLAGS)
AM_CXXFLAGS = $(PTHREAD_CFLAGS)
LDADD = -lqsesi -lqsecmn $(PTHREAD_LIBS)

if WIN32 
if WCHAR
LDADD += $(UNICOWS_LIBS)
endif
endif

dir01_SOURCES = dir01.c
fio01_SOURCES = fio01.c
fio02_SOURCES = fio02.c
fs01_SOURCES = fs01.c
fs02_SOURCES = fs02.c
fs03_SOURCES = fs03.c
glob01_SOURCES = glob01.c
log01_SOURCES = log01.c
nwad01_SOURCES = nwad01.c
nwif01_SOURCES = nwif01.c
nwif02_SOURCES = nwif02.c
pio01_SOURCES = pio01.c
rwl01_SOURCES = rwl01.c
sio01_SOURCES = sio01.c
sio02_SOURCES = sio02.c
sio03_SOURCES = sio03.c
spl01_SOURCES = spl01.c 
task01_SOURCES = task01.c
thr01_SOURCES = thr01.c



if ENABLE_CXX

bin_PROGRAMS += sck01 spl02 tcpsvr01 tcpsvr02 thr02 thr03

sck01_SOURCES = sck01.cpp
spl02_SOURCES = spl02.cpp
tcpsvr01_SOURCES = tcpsvr01.cpp
tcpsvr02_SOURCES = tcpsvr02.cpp
thr02_SOURCES = thr02.cpp
thr03_SOURCES = thr03.cpp

tcpsvr02_LDADD = $(LDADD) -L../../lib/sttp -lqsesttp

endif

