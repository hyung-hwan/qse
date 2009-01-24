
AM_CPPFLAGS = -I$(top_srcdir)/include

bin_PROGRAMS = qseawk

qseawk_SOURCES = awk.c
qseawk_LDFLAGS = -L../../lib/awk
qseawk_LDADD = -lqseawk $(LIBM)

if ENABLE_CXX
bin_PROGRAMS += qseawk++
qseawk___SOURCES = Awk.cpp
qseawk___LDFLAGS = -L../../lib/awk
qseawk___LDADD = -lqseawk++ $(LIBM)
endif
