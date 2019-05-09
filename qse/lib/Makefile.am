if ENABLE_STATIC_MODULE
SUBDIRS = cmn cry si sed awkmod awk xli http rad dhcp sttp
else
SUBDIRS = cmn cry si sed awk awkmod xli http rad dhcp sttp
endif
DIST_SUBDIRS = $(SUBDIRS) 

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = qsecmn.pc qsecry.pc qsesi.pc qsesed.pc qseawk.pc qsexli.pc qsehttp.pc qserad.pc qsedhcp.pc
