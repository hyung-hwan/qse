# EXTRA_DIST = 
SUBDIRS = cmn awk cut sed scm utl

pkgincludedir = $(includedir)/qse

pkginclude_HEADERS = config.h conf_msw.h conf_vms.h types.h macros.h pack1.h unpack.h 

if ENABLE_CXX
pkginclude_HEADERS += Types.hpp 
endif

install-data-hook:
	@$(GREP) -E "#define[ ]+QSE_" "$(srcdir)/config.h" > "$(DESTDIR)$(pkgincludedir)/config.h" 
	@$(RM) "$(DESTDIR)$(pkgincludedir)/config.h.in"
	@$(SED) 's|/\*#define QSE_HAVE_CONFIG_H\*/|#define QSE_HAVE_CONFIG_H|' "$(srcdir)/types.h" > "$(DESTDIR)$(pkgincludedir)/types.h" 

uninstall-hook:
	@$(RM) "$(DESTDIR)$(pkgincludedir)/config.h"
