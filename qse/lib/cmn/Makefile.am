
AUTOMAKE_OPTIONS = nostdinc
AM_CPPFLAGS = \
	-I$(top_builddir)/include \
	-I$(top_srcdir)/include \
	-I$(includedir)

lib_LTLIBRARIES = libqsecmn.la
libqsecmn_la_SOURCES = \
	syscall.h mem.h \
	mem.c xma.c fma.c chr.c chr_cnv.c rex.c \
	str_bas.c str_cat.c str_chr.c str_cnv.c str_cmp.c str_cpy.c str_dup.c \
	str_dyn.c str_fcpy.c str_pbrk.c str_put.c str_spn.c str_subst.c \
	str_utl.c \
	lda.c oht.c htb.c rbt.c sll.c gdl.c dll.c opt.c \
	tio.c tio_get.c tio_put.c \
	fio.c pio.c sio.c \
	alg_search.c  \
	alg_sort.c \
	time.c \
	misc.c \
	assert.c \
	main.c \
	stdio.c
libqsecmn_la_LDFLAGS = -L$(libdir) -version-info 1:0:0 -no-undefined

if WIN32 
libqsecmn_la_LIBADD = -lpsapi
endif

if ENABLE_CXX

lib_LTLIBRARIES += libqsecmnxx.la
libqsecmnxx_la_SOURCES = \
	Mmgr.cpp StdMmgr.cpp
libqsecmnxx_la_LDFLAGS = -L$(libdir) -version-info 1:0:0 -no-undefined

endif


