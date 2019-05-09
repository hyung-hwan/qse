AUTOMAKE_OPTION = foreign
ACLOCAL_AMFLAGS = -I m4

EXTRA_DIST = LICENSE README \
	bld/qse.bkl bld/readme.txt bld/os2.bkgen bld/win32.bkgen \
	bld/os2-watcom/makefile bld/win32-watcom/makefile \
	bld/win32-msvc/makefile bld/win32-borland/makefile


if ENABLE_STATIC_MODULE
SUBDIRS = include mod lib cmd
else
SUBDIRS = include lib mod cmd
endif

DIST_SUBDIRS = $(SUBDIRS) regress samples doc tools

distclean-local:
	$(RM) -r $(top_srcdir)/autom4te.cache
