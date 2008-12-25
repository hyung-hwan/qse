
AUTOMAKE_OPTIONS = nostdinc

AM_CPPFLAGS = -I$(top_srcdir)/include

lib_LTLIBRARIES = libqselsp.la 

libqselsp_la_SOURCES = lsp.c err.c env.c eval.c mem.c mem.h misc.c name.c prim.c prim_compar.c prim.h prim_let.c prim_math.c prim_prog.c print.c read.c lsp_i.h env.h misc.h name.h obj.h 
libqselsp_la_LDFLAGS = -L../cmn -version-info 1:0:0 -no-undefined
libqselsp_la_LIBADD = -lqsecmn
