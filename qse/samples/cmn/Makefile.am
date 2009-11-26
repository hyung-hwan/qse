AM_CPPFLAGS = -I$(top_srcdir)/include -DNDEBUG

bin_PROGRAMS = chr str sll map lda fio pio sio time rex01

LDFLAGS = -L../../lib/cmn 
LDADD = -lqsecmn 

chr_SOURCES = chr.c
str_SOURCES = str.c
sll_SOURCES = sll.c
map_SOURCES = map.c
lda_SOURCES = lda.c
fio_SOURCES = fio.c
pio_SOURCES = pio.c
sio_SOURCES = sio.c
time_SOURCES = time.c
rex01_SOURCES = rex01.c

if ENABLE_CXX

bin_PROGRAMS += rex02
rex02_SOURCES = rex02.cpp
rex02_CXXFLAGS = -I/usr/lib/wx/include/gtk2-unicode-release-2.8 -I/usr/include/wx-2.8 -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -pthread 
rex02_LDFLAGS = -pthread -Wl,-Bsymbolic-functions  -lwx_gtk2ud_richtext-2.8 -lwx_gtk2ud_aui-2.8 -lwx_gtk2ud_xrc-2.8 -lwx_gtk2ud_qa-2.8 -lwx_gtk2ud_html-2.8 -lwx_gtk2ud_adv-2.8 -lwx_gtk2ud_core-2.8 -lwx_baseud_xml-2.8 -lwx_baseud_net-2.8 -lwx_baseud-2.8 

endif
