AC_PREREQ(2.59)
AC_INIT(xpkit, deb-0.1.0, bacon@dreamwiz.com)
#AC_CONFIG_HEADER([config.h])

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC

# Checks for libraries.

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([fcntl.h limits.h stddef.h stdlib.h string.h unistdx.h])
AC_CHECK_HEADER([wchar.h], [AC_DEFINE(XP_HAVE_WCHAR_H)])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST

# Checks for library functions.
AC_FUNC_MEMCMP
AC_FUNC_REALLOC
AC_CHECK_FUNCS([memset])

AC_CONFIG_FILES([build/Makefile])
AC_OUTPUT
