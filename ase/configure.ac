AC_PREREQ(2.59)
AC_INIT(xpkit, deb-0.1.0)

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC

# Checks for libraries.

# Checks for header files.
#AC_HEADER_STDC
AC_CHECK_HEADER([unistd.h], [AC_DEFINE(XP_HAVE_UNISTD_H)])
AC_CHECK_HEADER([fcntl.h], [AC_DEFINE(XP_HAVE_FCNTL_H)])
AC_CHECK_HEADER([stdarg.h], [AC_DEFINE(XP_HAVE_STDARG_H)])
AC_CHECK_HEADER([stddef.h], [AC_DEFINE(XP_HAVE_STDDEF_H)])
AC_CHECK_HEADER([limits.h], [AC_DEFINE(XP_HAVE_LIMITS_H)])
AC_CHECK_HEADER([wchar.h], [AC_DEFINE(XP_HAVE_WCHAR_H)])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST

# Checks for library functions.
AC_FUNC_MEMCMP
AC_FUNC_REALLOC
AC_CHECK_FUNCS([memset])

AC_CONFIG_FILES([build/Makefile])
AC_OUTPUT
