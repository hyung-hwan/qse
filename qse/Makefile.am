
ACLOCAL_AMFLAGS = -I ac/m4
AUTOMAKE_OPTION = foreign
EXTRA_DIST = LICENSE

SUBDIRS = lib cmd include
DIST_SUBDIRS = $(SUBDIRS) regress samples doc

#install-data-hook:
#	cd include && $(MAKE) install

#uninstall-hook:
#	cd include && $(MAKE) uninstall

distclean-local:
	$(RM) -r autom4te.cache
