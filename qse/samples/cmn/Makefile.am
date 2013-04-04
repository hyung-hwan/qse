AUTOMAKE_OPTIONS = nostdinc

AM_CPPFLAGS = \
	-I$(top_builddir)/include \
	-I$(top_srcdir)/include \
	-I$(includedir)


bin_PROGRAMS = \
	chr01 \
	dir01 \
	dll \
	env01 \
	fio01 \
	fio02 \
	fma \
	fmt01 \
	fmt02 \
	fs01 \
	htb \
	glob01 \
	ipad01 \
	lda \
	main01 \
	main02 \
	mbwc01 \
	mbwc02 \
	nwad01 \
	nwif01 \
	nwif02 \
	oht \
	path01 \
	pio \
	pma \
	rex01 \
	rbt \
	sio01 \
	sio02 \
	sio03 \
	sll \
	slmb01 \
	str01 \
	task01 \
	time \
	tre01 \
	uri01 \
	xma 
	
LDFLAGS = -L../../lib/cmn 
LDADD = -lqsecmn 

if WIN32 
if WCHAR
LDADD += $(UNICOWS_LIBS)
endif
endif

chr01_SOURCES = chr01.c
env01_SOURCES = env01.c
dll_SOURCES = dll.c
fio01_SOURCES = fio01.c
fio02_SOURCES = fio02.c
fma_SOURCES = fma.c
fmt01_SOURCES = fmt01.c
fmt02_SOURCES = fmt02.c
fs01_SOURCES = fs01.c
glob01_SOURCES = glob01.c
htb_SOURCES = htb.c
ipad01_SOURCES = ipad01.c
lda_SOURCES = lda.c
main01_SOURCES = main01.c
main02_SOURCES = main02.c
mbwc01_SOURCES = mbwc01.c
mbwc02_SOURCES = mbwc02.c
nwad01_SOURCES = nwad01.c
oht_SOURCES = oht.c
path01_SOURCES = path01.c
pio_SOURCES = pio.c
pma_SOURCES = pma.c
rex01_SOURCES = rex01.c
rbt_SOURCES = rbt.c
sio01_SOURCES = sio01.c
sio02_SOURCES = sio02.c
sio03_SOURCES = sio03.c
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

endif
