
AUTOMAKE_OPTIONS = nostdinc
AM_CFLAGS = -I$(top_builddir)/include

lib_LTLIBRARIES = libqsecmn.la
libqsecmn_la_SOURCES = mem.h chr.h \
	mem.c chr.c chr_cnv.c rex.c str_bas.c str_cnv.c str_dyn.c str_utl.c \
	lda.c map.c sll.c dll.c opt.c \
	fio.c pcp.c sio.c tio.c tio_get.c tio_put.c \
	time.c \
	misc.c
libqsecmn_la_LDFLAGS = -version-info 1:0:0 -no-undefined

if WIN32 
libqsecmn_la_LIBADD = -lpsapi
endif

