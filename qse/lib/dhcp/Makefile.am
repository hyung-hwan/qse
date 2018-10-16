AUTOMAKE_OPTIONS = nostdinc

AM_CPPFLAGS = \
	-I$(top_builddir)/include \
	-I$(top_srcdir)/include

#noinst_HEADERS =

lib_LTLIBRARIES = libqsedhcp.la
libqsedhcp_la_SOURCES =  \
	dhcp4msg.c dhcp6msg.c

libqsedhcp_la_CFLAGS = 
libqsedhcp_la_LDFLAGS = -L../cmn -version-info 1:0:0 -no-undefined
libqsedhcp_la_LIBADD = -lqsecmn

