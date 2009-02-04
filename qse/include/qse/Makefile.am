
# EXTRA_DIST = README
SUBDIRS = cmn utl awk lsp 

pkginclude_HEADERS = config.h.in conf_msw.h conf_vms.h types.h macros.h pack1.h unpack.h

pkgincludedir= $(includedir)/qse

CLEANFILES = *dist

install-data-hook:
	@"$(GREP)" -E "#define[ ]+QSE_" "$(srcdir)/config.h" > "$(DESTDIR)$(pkgincludedir)/config.h" 
	@"$(RM)" -f "$(DESTDIR)$(pkgincludedir)/config.h.in"
	@"$(SED)" 's|/\*#define QSE_HAVE_CONFIG_H\*/|#define QSE_HAVE_CONFIG_H|' "$(srcdir)/types.h" > "$(DESTDIR)$(pkgincludedir)/types.h" 

uninstall-hook:
	@"$(RM)" -f "$(DESTDIR)$(pkgincludedir)/config.h"
