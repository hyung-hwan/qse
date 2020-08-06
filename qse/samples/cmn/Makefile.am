AUTOMAKE_OPTIONS = nostdinc

AM_CPPFLAGS = \
	-I$(top_builddir)/include \
	-I$(top_srcdir)/include \
	-I$(includedir)


bin_PROGRAMS = \
	arr01 \
	chr01 \
	dll \
	env01 \
	fma \
	fmt01 \
	fmt02 \
	htb01 \
	htl01 \
	ipad01 \
	main01 \
	main02 \
	mbwc01 \
	mbwc02 \
	oht \
	path01 \
	pma \
	rex01 \
	rbt01 \
	sll \
	slmb01 \
	str01 \
	str02 \
	time \
	tre01 \
	uri01 \
	xma 
	
AM_CFLAGS = $(PTHREAD_CFLAGS)
AM_CXXFLAGS = $(PTHREAD_CFLAGS)
AM_LDFLAGS = -L../../lib/si  -L../../lib/cmn
LDADD = -lqsesi -lqsecmn $(LIBM) $(PTHREAD_LIBS)

if WIN32 
if WCHAR
LDADD += $(UNICOWS_LIBS)
endif
endif

chr01_SOURCES = chr01.c
env01_SOURCES = env01.c
dll_SOURCES = dll.c
fma_SOURCES = fma.c
fmt01_SOURCES = fmt01.c
fmt02_SOURCES = fmt02.c
htb01_SOURCES = htb01.c
ipad01_SOURCES = ipad01.c
arr01_SOURCES = arr01.c
main01_SOURCES = main01.c
main02_SOURCES = main02.c
mbwc01_SOURCES = mbwc01.c
mbwc02_SOURCES = mbwc02.c
oht_SOURCES = oht.c
path01_SOURCES = path01.c
pma_SOURCES = pma.c
rex01_SOURCES = rex01.c
rbt01_SOURCES = rbt01.c
sll_SOURCES = sll.c
slmb01_SOURCES = slmb01.c
str01_SOURCES = str01.c
time_SOURCES = time.c
tre01_SOURCES = tre01.c
xma_SOURCES = xma.c

if ENABLE_CXX

#bin_PROGRAMS += rex02
#rex02_SOURCES = rex02.cpp
#rex02_CXXFLAGS = -I/usr/lib/wx/include/gtk2-unicode-release-2.8 -I/usr/include/wx-2.8 -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -pthread 
#rex02_LDFLAGS = -pthread -Wl,-Bsymbolic-functions  -lwx_gtk2ud_richtext-2.8 -lwx_gtk2ud_aui-2.8 -lwx_gtk2ud_xrc-2.8 -lwx_gtk2ud_qa-2.8 -lwx_gtk2ud_html-2.8 -lwx_gtk2ud_adv-2.8 -lwx_gtk2ud_core-2.8 -lwx_baseud_xml-2.8 -lwx_baseud_net-2.8 -lwx_baseud-2.8 

bin_PROGRAMS += arr02 arr03 bh01 bh02 hl01 htb02 rbt02 rbt03 sp01 sp02 str02

arr02_SOURCES = arr02.cpp

arr03_SOURCES = arr03.cpp

bh01_SOURCES = bh01.cpp

bh02_SOURCES = bh02.cpp

hl01_SOURCES = hl01.cpp

htb02_SOURCES = htb02.cpp

rbt02_SOURCES = rbt02.cpp #RedBlackTree

rbt03_SOURCES = rbt03.cpp #RedBlackTable

sp01_SOURCES = sp01.cpp # ScopedPtr

sp02_SOURCES = sp02.cpp # SharedPtr

str02_SOURCES = str02.cpp # SharedPtr

endif
