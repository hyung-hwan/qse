
AM_CPPFLAGS = -I$(top_srcdir)/include

bin_PROGRAMS = qselsp 

qselsp_SOURCES = lsp.c
qselsp_LDFLAGS = -L../../lib/lsp -L../../lib/utl 
qselsp_LDADD = -lqseutl -lqselsp $(LIBM)

