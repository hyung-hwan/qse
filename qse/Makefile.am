AUTOMAKE_OPTION = foreign
ACLOCAL_AMFLAGS = -I m4

EXTRA_DIST = LICENSE README \
	bld/qse.bkl bld/readme.txt bld/os2.bkgen bld/win32.bkgen \
	bld/os2-watcom/makefile bld/win32-watcom/makefile \
	bld/win32-msvc/makefile bld/win32-borland/makefile

SUBDIRS = include lib cmd

DIST_SUBDIRS = $(SUBDIRS) regress samples doc tools

distclean-local:
	$(RM) -r $(top_srcdir)/autom4te.cache

######################################################################

rpm: dist-gzip
	mkdir -p "@abs_builddir@/pkgs/RPM/BUILD"
	mkdir -p "@abs_builddir@/pkgs/RPM/SOURCES"
	mkdir -p "@abs_builddir@/pkgs/RPM/SRPMS"
	mkdir -p "@abs_builddir@/pkgs/RPM/RPMS"
	cp @PACKAGE_NAME@-@PACKAGE_VERSION@.tar.gz "@abs_builddir@/pkgs/RPM/SOURCES"
	rpmbuild --define "_topdir @abs_builddir@/pkgs/RPM" -ba @abs_builddir@/pkgs/qse.spec --target=@build_cpu@

