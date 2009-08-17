
ACLOCAL_AMFLAGS = -I ac/m4
AUTOMAKE_OPTION = foreign
EXTRA_DIST = ac/au ac/m4

SUBDIRS = lib cmd
DIST_SUBDIRS = $(SUBDIRS) include regress samples doc

install-data-hook:
	cd include && $(MAKE) install

uninstall-hook:
	cd include && $(MAKE) uninstall

distclean-local:
	$(RM) -r autom4te.cache
