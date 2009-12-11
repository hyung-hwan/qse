
AUTOMAKE_OPTIONS = nostdinc
AM_CPPFLAGS = -I$(top_srcdir)/include 

lib_LTLIBRARIES = libqsecmn.la
libqsecmn_la_SOURCES = \
	syscall.h mem.h \
	mem.c chr.c chr_cnv.c rex.c \
	str_bas.c str_cnv.c str_dyn.c str_utl.c \
	lda.c map.c sll.c dll.c opt.c \
	tio.c tio_get.c tio_put.c \
	fio.c pio.c sio.c \
	time.c \
	misc.c \
	assert.c \
	main.c \
	stdio.c
libqsecmn_la_LDFLAGS = -version-info 1:0:0 -no-undefined

if WIN32 
libqsecmn_la_LIBADD = -lpsapi
endif

