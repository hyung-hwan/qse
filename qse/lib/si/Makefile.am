AUTOMAKE_OPTIONS = nostdinc

AM_CPPFLAGS = \
	-I$(top_builddir)/include \
	-I$(top_srcdir)/include

lib_LTLIBRARIES = libqsesi.la
libqsesi_la_SOURCES =  \
	aio-prv.h \
	aio.c \
	aio-pro.c  \
	aio-sck.c  \
	aio-tmr.c  \
	cnd.c \
	fio.c  \
	intr.c \
	mtx.c \
	mux.c \
	nwif.c \
	nwio.c  \
	pio.c  \
	rwl.c \
	sio.c  \
	task.c \
	thr.c \
	thr.h \
	tio.c

libqsesi_la_CFLAGS = $(PTHREAD_CFLAGS)
libqsesi_la_LDFLAGS = -L../cmn -version-info 1:0:0 -no-undefined
libqsesi_la_LIBADD = -lqsecmn $(PTHREAD_LIBS)

if ENABLE_CXX
lib_LTLIBRARIES += libqsesixx.la
libqsesixx_la_SOURCES = \
	SocketAddress.cpp \
	Socket.cpp
libqsesixx_la_LDFLAGS = -L. -L../cmn -version-info 1:0:0 -no-undefined
libqsesixx_la_LIBADD = -lqsecmnxx -lqsecmn 
endif

