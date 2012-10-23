AUTOMAKE_OPTION = foreign
ACLOCAL_AMFLAGS = -I m4

EXTRA_DIST = LICENSE
SUBDIRS = libltdl include lib cmd mod
DIST_SUBDIRS = $(SUBDIRS) regress samples doc tools

distclean-local:
	$(RM) -r autom4te.cache
