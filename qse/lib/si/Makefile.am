AUTOMAKE_OPTIONS = nostdinc

AM_CPPFLAGS = \
	-I$(top_builddir)/include \
	-I$(top_srcdir)/include

noinst_HEADERS = \
	fs-prv.h \
	glob-imp.h

lib_LTLIBRARIES = libqsesi.la
libqsesi_la_SOURCES =  \
	cnd.c \
	dir.c \
	fio.c  \
	fs.c \
	fs-attr.c \
	fs-copy.c \
	fs-delete.c \
	fs-err.c \
	fs-make.c \
	fs-move.c \
	fs-util.c \
	glob.c \
	intr.c \
	log.c \
	mtx.c \
	mux.c \
	netlink.c \
	nwad.c \
	nwad-skad.c \
	nwif.c \
	nwif-cfg.c \
	nwio.c  \
	os.c \
	pio.c  \
	rwl.c \
	sck.c \
	sinfo.c \
	sio.c  \
	task.c \
	thr.c \
	thr-prv.h \
	tio.c

libqsesi_la_CFLAGS = $(PTHREAD_CFLAGS)
libqsesi_la_LDFLAGS = -L../cmn -version-info 1:0:0 -no-undefined
libqsesi_la_LIBADD = -lqsecmn $(PTHREAD_LIBS) $(SSL_LIBS)
libqsesi_la_DEPENDENCIES = ../../lib/cmn/libqsecmn.la

if ENABLE_CXX
lib_LTLIBRARIES += libqsesixx.la
libqsesixx_la_SOURCES = \
	App.cpp \
	Path.cpp \
	SocketAddress.cpp \
	Socket.cpp \
	TcpServer.cpp \
	Thread.cpp
libqsesixx_la_LDFLAGS = -L. -L../cmn -version-info 1:0:0 -no-undefined
libqsesixx_la_LIBADD = -lqsecmnxx -lqsesi -lqsecmn 
libqsesixx_la_DEPENDENCIES = ../../lib/cmn/libqsecmnxx.la libqsesi.la ../../lib/cmn/libqsecmn.la
endif

