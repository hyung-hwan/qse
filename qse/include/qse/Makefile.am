SUBDIRS = cmn awk cut sed fs net stx

pkgincludedir = $(includedir)/qse

pkginclude_HEADERS = \
	conf_msw.h conf_os2.h conf_dos.h conf_vms.h \
	types.h macros.h pack1.h unpack.h 

if ENABLE_CXX
pkginclude_HEADERS += Types.hpp 
endif

install-data-hook:
	@$(ECHO) "#ifndef _QSE_CONFIG_H_" > "$(DESTDIR)$(pkgincludedir)/config.h"
	@$(ECHO) "#define _QSE_CONFIG_H_" >> "$(DESTDIR)$(pkgincludedir)/config.h"
	@$(EGREP) "#define[ ]+QSE_" "$(top_builddir)/include/qse/config.h" >> "$(DESTDIR)$(pkgincludedir)/config.h" 
	@$(ECHO) "#endif" >> "$(DESTDIR)$(pkgincludedir)/config.h"
	@$(RM) "$(DESTDIR)$(pkgincludedir)/config.h.in"
	@$(SED) 's|/\*#define QSE_HAVE_CONFIG_H\*/|#define QSE_HAVE_CONFIG_H|' "$(srcdir)/types.h" > "$(DESTDIR)$(pkgincludedir)/types.h" 

uninstall-hook:
	@$(RM) "$(DESTDIR)$(pkgincludedir)/config.h"
