AC_PREREQ(2.53)
AC_INIT([xpkit], [deb-0.1.0])
AC_REVISION([$Revision: 1.27 $])
AC_CONFIG_HEADER([xp/config.h])

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AC_PROG_RANLIB

# Checks for libraries.
AC_CHECK_LIB(pthread, pthread_create)
AC_CHECK_LIB(nsl, gethostbyname)
AC_CHECK_LIB(socket, connect)
AC_CHECK_LIB(rt, nanosleep)

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

# Checks for data types
AC_CHECK_TYPE(long long, [AC_DEFINE([XP_HAVE_LONG_LONG],[],[long long])])
AC_CHECK_TYPE(long double, [AC_DEFINE([XP_HAVE_LONG_DOUBLE],[],[long double])])

AC_CHECK_SIZEOF(char)
AC_CHECK_SIZEOF(short)
AC_CHECK_SIZEOF(int)
AC_CHECK_SIZEOF(long)
AC_CHECK_SIZEOF(long long)
AC_CHECK_SIZEOF(__int8)
AC_CHECK_SIZEOF(__int16)
AC_CHECK_SIZEOF(__int32)
AC_CHECK_SIZEOF(__int64)
AC_CHECK_SIZEOF(__int128)
AC_CHECK_SIZEOF(void *)
AC_CHECK_SIZEOF(float)
AC_CHECK_SIZEOF(double)
AC_CHECK_SIZEOF(long double)

#AC_CHECK_FUNC(nanosleep,[AC_DEFINE([XP_HAVE_NANOSLEEP])],
#	[AC_CHECK_LIB(rt,nanosleep, LIBS="${LIBS} -lrt", 	 
#		[AC_CHECK_LIB(posix4,nanosleep, LIBS="${LIBS} -lposix4")])]) 	 

# Checks for library functions.
#AC_FUNC_MEMCMP
#AC_FUNC_REALLOC
#AC_CHECK_FUNCS([memset])

# Checks for system files
AC_CHECK_FILE([/NextDeveloper],[AC_DEFINE([_POSIX_SOURCE],[],[_POSIX_SOURCE])])

AC_CONFIG_FILES([
	Makefile utils/Makefile xp/Makefile 
	xp/c/Makefile xp/mp/Makefile xp/lisp/Makefile 
	xp/test/c/Makefile xp/test/lisp/Makefile
	xpp/sys/Makefile xpp/test/sys/Makefile])
AC_OUTPUT
