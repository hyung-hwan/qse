
AUTOMAKE_OPTION = foreign
# EXTRA_DIST = 

SUBDIRS = lib cmd
DIST_SUBDIRS = $(__COREDIRS) include test

install-data-hook:
	cd include && $(MAKE) install

uninstall-hook:
	cd include && $(MAKE) uninstall
