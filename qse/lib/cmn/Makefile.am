
AUTOMAKE_OPTIONS = nostdinc
AM_CPPFLAGS = \
	-I$(top_builddir)/include \
	-I$(top_srcdir)/include \
	-I$(includedir)


lib_LTLIBRARIES = libqsecmn.la

noinst_HEADERS = \
	cp949.h \
	cp950.h \
	fs.h \
	mem.h \
	syscall.h \
	tre.h \
	tre-ast.h \
	tre-compile.h \
	tre-match-utils.h \
	tre-parse.h \
	tre-stack.h

libqsecmn_la_SOURCES = \
	alg-base64.c \
	alg-rand.c \
	alg-search.c  \
	alg-sort.c \
	assert.c \
	chr.c \
	cp949.c \
	cp950.c \
	dll.c \
	env.c \
	gdl.c \
	htb.c \
	fio.c \
	fma.c \
	fmt.c \
	fs.c \
	fs-err.c \
	fs-move.c \
	glob.c \
	hton.c \
	ipad.c \
	lda.c \
	main.c \
	mbwc.c \
	mbwc-str.c \
	mem.c \
	nwad.c \
	nwif.c \
	nwio.c \
	oht.c \
	opt.c \
	path-basename.c \
	path-canon.c \
	pio.c \
	pma.c \
	rbt.c \
	rex.c \
	sio.c \
	sll.c \
	slmb.c \
	stdio.c \
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
	str-fnmat.c \
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
	time.c \
	tio.c \
	tre.c \
	tre-ast.c \
	tre-compile.c \
	tre-match-backtrack.c \
	tre-match-parallel.c \
	tre-parse.c \
	tre-stack.c \
	uri.c \
	utf8.c \
	xma.c

libqsecmn_la_LDFLAGS = -L$(libdir) -version-info 1:0:0 -no-undefined
libqsecmn_la_LIBADD = $(SOCKET_LIBS)

if ENABLE_CXX

lib_LTLIBRARIES += libqsecmnxx.la
libqsecmnxx_la_SOURCES = \
	Mmgr.cpp StdMmgr.cpp
libqsecmnxx_la_LDFLAGS = -L$(libdir) -version-info 1:0:0 -no-undefined
libqsecmnxx_la_LIBADD = 

endif


