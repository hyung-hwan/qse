AUTOMAKE_OPTION = foreign
ACLOCAL_AMFLAGS = -I m4

EXTRA_DIST = LICENSE \
	bld/qse.bkl bld/readme.txt bld/os2.bkgen bld/win32.bkgen \
	bld/os2-watcom/makefile bld/win32-watcom/makefile \
	bld/win32-msvc/makefile bld/win32-borland/makefile
SUBDIRS = libltdl include lib cmd
DIST_SUBDIRS = $(SUBDIRS) regress samples doc tools

distclean-local:
	$(RM) -r autom4te.cache
