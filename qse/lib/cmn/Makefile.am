
AUTOMAKE_OPTIONS = nostdinc
AM_CPPFLAGS = \
	-I$(top_builddir)/include \
	-I$(top_srcdir)/include \
	-I$(includedir)


lib_LTLIBRARIES = libqsecmn.la

libqsecmn_la_SOURCES = \
	tre.h \
	alg-search.c  \
	alg-sort.c \
	assert.c \
	chr.c \
	chr-cnv.c \
	dll.c \
	env.c \
	gdl.c \
	htb.c \
	lda.c \
	fio.c \
	fma.c \
	main.c \
	mem.c \
	mem.h \
	misc.c \
	oht.c \
	opt.c \
	pma.c \
	pio.c \
	rbt.c \
	rex.c \
	sio.c \
	sll.c \
	str-beg.c \
	str-cat.c \
	str-chr.c \
	str-cnv.c \
	str-cmp.c \
	str-cpy.c \
	str-del.c \
	str-dup.c \
	str-dynm.c \
	str-dynw.c \
	str-end.c \
	str-excl.c \
	str-fcpy.c \
	str-incl.c \
	str-len.c \
	str-pac.c \
	str-pbrk.c \
	str-put.c \
	str-rev.c \
	str-rot.c \
	str-set.c \
	str-spl.c \
	str-spn.c \
	str-str.c \
	str-subst.c \
	str-tok.c \
	str-trm.c \
	str-word.c \
	syscall.h \
	time.c \
	tio.c \
	tio-get.c \
	tio-put.c \
	tre.c \
	tre.h \
	tre-ast.c \
	tre-ast.h \
	tre-compile.c \
	tre-compile.h \
	tre-match-backtrack.c \
	tre-match-parallel.c \
	tre-match-utils.h \
	tre-parse.c \
	tre-parse.h \
	tre-stack.c \
	tre-stack.h \
	stdio.c \
	xma.c

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


