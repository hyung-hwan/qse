AUTOMAKE_OPTIONS = nostdinc

CPPFLAGS_ALL_COMMON = \
	-I$(top_builddir)/include \
	-I$(top_srcdir)/include

LDFLAGS_ALL_COMMON = -L. -L../cmn -version-info 1:0:0 -no-undefined

##################################################
# MAIN LIBRARY 
##################################################

CPPFLAGS_LIB_COMMON = $(CPPFLAGS_ALL_COMMON)
LDFLAGS_LIB_COMMON = $(LDFLAGS_ALL_COMMON) 
LIBADD_LIB_COMMON = -lqsecmn 

lib_LTLIBRARIES = libqseio.la
libqseio_la_SOURCES = \
	aio-prv.h \
	aio.c \
	aio-pro.c  \
	aio-sck.c  \
	aio-tmr.c  \
	fio.c  \
	nwio.c  \
	pio.c  \
	sio.c  \
	tio.c
libqseio_la_CPPFLAGS = $(CPPFLAGS_LIB_COMMON)
libqseio_la_LDFLAGS = $(LDFLAGS_LIB_COMMON)
libqseio_la_LIBADD = $(LIBADD_LIB_COMMON) $(SOCKET_LIBS) $(SSL_LIBS)
