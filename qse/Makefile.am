
ACLOCAL_AMFLAGS = -I ac/m4
AUTOMAKE_OPTION = foreign
# EXTRA_DIST = 

SUBDIRS = lib cmd
DIST_SUBDIRS = $(SUBDIRS) include test doc

install-data-hook:
	cd include && $(MAKE) install

uninstall-hook:
	cd include && $(MAKE) uninstall
