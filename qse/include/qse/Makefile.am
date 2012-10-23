SUBDIRS = cmn awk sed net

pkgincludedir = $(includedir)/qse

pkginclude_HEADERS = \
	conf-msw.h conf-os2.h conf-dos.h conf-vms.h conf-inf.h \
	types.h macros.h pack1.h unpack.h 

if ENABLE_CXX
pkginclude_HEADERS += Types.hpp 
endif

install-data-hook:
	@echo "#ifndef _QSE_CONFIG_H_" > "$(DESTDIR)$(pkgincludedir)/config.h"
	@echo "#define _QSE_CONFIG_H_" >> "$(DESTDIR)$(pkgincludedir)/config.h"
	@$(EGREP) "#define[ ]+QSE_" "$(top_builddir)/include/qse/config.h" >> "$(DESTDIR)$(pkgincludedir)/config.h" 
	@echo "#endif" >> "$(DESTDIR)$(pkgincludedir)/config.h"
	@$(RM) "$(DESTDIR)$(pkgincludedir)/config.h.in"
	@$(SED) 's|/\*#define QSE_HAVE_CONFIG_H\*/|#define QSE_HAVE_CONFIG_H|' "$(srcdir)/types.h" > "$(DESTDIR)$(pkgincludedir)/types.h" 

uninstall-hook:
	@$(RM) "$(DESTDIR)$(pkgincludedir)/config.h"
