AUTOMAKE_OPTIONS = nostdinc

AM_CPPFLAGS = \
	-I$(top_builddir)/include \
	-I$(top_srcdir)/include \
	-I$(includedir)

bin_PROGRAMS =
	
AM_LDFLAGS = -L../../lib/xli -L../../lib/sttp -L../../lib/si  -L../../lib/cmn 
LDADD = -lqsexli -lqsesttp -lqsesi -lqsecmn 

if WIN32 
LDADD += $(UNICOWS_LIBS)
endif

if ENABLE_CXX

bin_PROGRAMS += app01

app01_SOURCES = Main.cpp MainApp.cpp MainApp.hpp TcpGate.cpp TcpGate.hpp

endif

