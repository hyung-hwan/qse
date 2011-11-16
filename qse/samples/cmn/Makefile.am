AUTOMAKE_OPTIONS = nostdinc

AM_CPPFLAGS = \
	-I$(top_builddir)/include \
	-I$(top_srcdir)/include \
	-I$(includedir)


bin_PROGRAMS = xma fma pma chr str sll dll lda oht htb rbt fio01 fio02 pio sio time main main2 rex01 env path01 tre01 fmt01 fmt02

LDFLAGS = -L../../lib/cmn 
LDADD = -lqsecmn 

xma_SOURCES = xma.c
fma_SOURCES = fma.c
pma_SOURCES = pma.c
chr_SOURCES = chr.c
str_SOURCES = str.c
sll_SOURCES = sll.c
dll_SOURCES = dll.c
lda_SOURCES = lda.c
oht_SOURCES = oht.c
htb_SOURCES = htb.c
rbt_SOURCES = rbt.c
fio01_SOURCES = fio01.c
fio02_SOURCES = fio02.c
pio_SOURCES = pio.c
sio_SOURCES = sio.c
time_SOURCES = time.c
main_SOURCES = main.c
main2_SOURCES = main2.c
rex01_SOURCES = rex01.c
env_SOURCES = env.c
path01_SOURCES = path01.c
tre01_SOURCES = tre01.c
fmt01_SOURCES = fmt01.c
fmt02_SOURCES = fmt02.c

if ENABLE_CXX

#bin_PROGRAMS += rex02
#rex02_SOURCES = rex02.cpp
#rex02_CXXFLAGS = -I/usr/lib/wx/include/gtk2-unicode-release-2.8 -I/usr/include/wx-2.8 -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -pthread 
#rex02_LDFLAGS = -pthread -Wl,-Bsymbolic-functions  -lwx_gtk2ud_richtext-2.8 -lwx_gtk2ud_aui-2.8 -lwx_gtk2ud_xrc-2.8 -lwx_gtk2ud_qa-2.8 -lwx_gtk2ud_html-2.8 -lwx_gtk2ud_adv-2.8 -lwx_gtk2ud_core-2.8 -lwx_baseud_xml-2.8 -lwx_baseud_net-2.8 -lwx_baseud-2.8 

endif
