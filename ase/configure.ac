AC_PREREQ(2.53)
AC_INIT([xpkit], [deb-0.1.0])
AC_REVISION([$Revision: 1.14 $])
AC_CONFIG_HEADER([xp/config.h])

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AC_PROG_RANLIB

# Checks for libraries.

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADER([unistd.h], [AC_DEFINE([XP_HAVE_UNISTD_H],[],[unistd.h])])
AC_CHECK_HEADER([fcntl.h], [AC_DEFINE([XP_HAVE_FCNTL_H],[],[fcntl.h])])
AC_CHECK_HEADER([stdarg.h], [AC_DEFINE([XP_HAVE_STDARG_H],[],[stdarg.h])])
AC_CHECK_HEADER([stddef.h], [AC_DEFINE([XP_HAVE_STDDEF_H],[],[stddef.h])])
AC_CHECK_HEADER([limits.h], [AC_DEFINE([XP_HAVE_LIMITS_H],[],[limits.h])])
AC_CHECK_HEADER([wchar.h], [AC_DEFINE([XP_HAVE_WCHAR_H],[],[wchar.h])])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_BIGENDIAN(
	[AC_DEFINE([XP_ENDIAN_BIG],[],[Big Endian])],
	[AC_DEFINE([XP_ENDIAN_LITTLE],[],[Little Endian])],
	[AC_DEFINE([XP_ENDIAN_UNKNOWN],[],[Unknown Endian])])

# Checks for library functions.
#AC_FUNC_MEMCMP
#AC_FUNC_REALLOC
#AC_CHECK_FUNCS([memset])

# Checks for system files
AC_CHECK_FILE([/NextDeveloper],[AC_DEFINE([_POSIX_SOURCE],[],[_POSIX_SOURCE])])

AC_CONFIG_FILES([
	Makefile build/Makefile xp/Makefile 
	xp/c/Makefile xp/lisp/Makefile 
	xp/test/c/Makefile xp/test/lisp/Makefile])
AC_OUTPUT
