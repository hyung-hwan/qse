
AUTOMAKE_OPTIONS = nostdinc
AM_CPPFLAGS = \
	-I$(top_builddir)/include \
	-I$(top_srcdir)/include

lib_LTLIBRARIES = libqsecmn.la

noinst_HEADERS = \
	cp949.h \
	cp950.h \
	fmt-prv.h \
	fmt-intmax.h \
	fmt-out.h \
	mem-prv.h \
	str-cat.h \
	str-dyn.h \
	str-fcpy.h \
	str-fmt.h \
	str-join.h \
	str-subst.h \
	syscall.h \
	syserr.h \
	tre.h \
	tre-ast.h \
	tre-compile.h \
	tre-match-ut.h \
	tre-parse.h \
	tre-stack.h \
	uni-case.h \
	uni-trait.h \
	va_copy.h

libqsecmn_la_SOURCES = \
	alg-base64.c \
	alg-rand.c \
	alg-search.c  \
	alg-sort.c \
	arr.c \
	assert.c \
	chr.c \
	dll.c \
	env.c \
	gdl.c \
	htb.c \
	htl.c \
	fma.c \
	fmt-intmax.c \
	fmt-out.c \
	hton.c \
	hwad.c \
	ipad.c \
	main.c \
	mb8.c \
	mbwc.c \
	mbwc-str.c \
	mem.c \
	oht.c \
	opt.c \
	opt-cli.c \
	path-base.c \
	path-canon.c \
	path-core.c \
	path-merge.c \
	pma.c \
	rbt.c \
	rex.c \
	sll.c \
	slmb.c \
	str-beg.c \
	str-cat.c \
	str-chr.c \
	str-cnv.c \
	str-cmp.c \
	str-cpy.c \
	str-del.c \
	str-dup.c \
	str-dyn.c \
	str-end.c \
	str-excl.c \
	str-fcpy.c \
	str-fmt.c \
	str-fnmat.c \
	str-incl.c \
	str-join.c \
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
	str-type.c \
	str-word.c \
	time.c \
	tmr.c \
	tre.c \
	tre-ast.c \
	tre-compile.c \
	tre-match-bt.c \
	tre-match-pa.c \
	tre-parse.c \
	tre-stack.c \
	uri.c \
	utf8.c \
	xma.c

if ENABLE_BUNDLED_UNICODE
libqsecmn_la_SOURCES += uni.c 
endif

if ENABLE_XCMGRS
libqsecmn_la_SOURCES += cp949.c cp950.c
endif

libqsecmn_la_LDFLAGS = -version-info 1:0:0 -no-undefined
libqsecmn_la_LIBADD = $(SOCKET_LIBS) $(QUADMATH_LIBS)

if ENABLE_CXX

libqsecmn_la_SOURCES += \
	ErrorGrab.cpp \
	Mmgr.cpp \
	StdMmgr.cpp \
	HeapMmgr.cpp \
	Mmged.cpp \
	Mpool.cpp \
	String.cpp
endif


