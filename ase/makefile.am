##############################################################################
#
# ASE 
# Copyright (c) Chung,Hyung-Hwan (bacon@abiyo.net)
#
##############################################################################

AUTOMAKE_OPTION = foreign
# EXTRA_DIST = 

SUBDIRS = lib cmd
DIST_SUBDIRS = $(SUBDIRS) include

install-data-hook:
	cd include && $(MAKE) install

uninstall-hook:
	cd include && $(MAKE) uninstall
