AC_PREREQ(2.53)
AC_INIT([ase], [deb-0.1.0])
AC_REVISION([$Revision: 1.72 $])
AC_CONFIG_HEADER([conf_unx.h])

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
AC_CHECK_HEADER([unistd.h], [AC_DEFINE([ASE_HAVE_UNISTD_H],[],[unistd.h])])
AC_CHECK_HEADER([fcntl.h], [AC_DEFINE([ASE_HAVE_FCNTL_H],[],[fcntl.h])])
AC_CHECK_HEADER([stdarg.h], [AC_DEFINE([ASE_HAVE_STDARG_H],[],[stdarg.h])])
AC_CHECK_HEADER([stddef.h], [AC_DEFINE([ASE_HAVE_STDDEF_H],[],[stddef.h])])
AC_CHECK_HEADER([limits.h], [AC_DEFINE([ASE_HAVE_LIMITS_H],[],[limits.h])])
AC_CHECK_HEADER([wchar.h], [AC_DEFINE([ASE_HAVE_WCHAR_H],[],[wchar.h])])
AC_CHECK_HEADER([poll.h], [AC_DEFINE([ASE_HAVE_POLL_H],[],[poll.h])])
AC_CHECK_HEADER([sys/epoll.h], [AC_DEFINE([ASE_HAVE_SYS_EPOLL_H],[],[sys/epoll.h])])
AC_CHECK_HEADER([pthread.h], [AC_DEFINE([ASE_HAVE_PTHREAD_H],[],[pthread.h])])

AC_CHECK_HEADER([sys/sysinfo.h],
	[AC_DEFINE([ASE_HAVE_SYS_SYSINFO_H],[],[sys/sysinfo.h])])

# Checks for structure members
AC_CHECK_HEADER([sys/sysinfo.h], [
	AC_CHECK_MEMBER(struct sysinfo.mem_unit,
		[AC_DEFINE([ASE_HAVE_SYS_SYSINFO_MEM_UNIT],[],[sys/sysinfo.mem_unit])],
		[],
		[#include <sys/sysinfo.h>])])

AC_CHECK_MEMBER(struct sockaddr_in6.sin6_scope_id,
	[AC_DEFINE([ASE_HAVE_IN6_SCOPE_ID],[],[in6.scope_id])],
	[],
	[#include <sys/types.h> 
	 #include <sys/socket.h> 
	 #include <netinet/in.h>])

AC_CHECK_MEMBER(struct sockaddr_storage.ss_family,
	[AC_DEFINE([ASE_HAVE_SS_FAMILY],[],[sockaddr_storage.ss_family])],
	[],
	[#include <sys/types.h> 
	 #include <sys/socket.h> 
	 #include <netdb.h>])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_BIGENDIAN(
	[AC_DEFINE([ASE_ENDIAN_BIG],[],[Big Endian])],
	[AC_DEFINE([ASE_ENDIAN_LITTLE],[],[Little Endian])],
	[AC_DEFINE([ASE_ENDIAN_UNKNOWN],[],[Unknown Endian])])

# Checks for data types
AC_CHECK_TYPE(long long, [AC_DEFINE([ASE_HAVE_LONG_LONG],[],[long long])])
AC_CHECK_TYPE(long double, [AC_DEFINE([ASE_HAVE_LONG_DOUBLE],[],[long double])])

AC_CHECK_SIZEOF(char)
AC_CHECK_SIZEOF(short)
AC_CHECK_SIZEOF(int)
AC_CHECK_SIZEOF(long)
AC_CHECK_SIZEOF(long long)
AC_CHECK_SIZEOF(__int8)
AC_CHECK_SIZEOF(__int16)
AC_CHECK_SIZEOF(__int32)
AC_CHECK_SIZEOF(__int64)
AC_CHECK_SIZEOF(__int96)
AC_CHECK_SIZEOF(__int128)
AC_CHECK_SIZEOF(void *)
AC_CHECK_SIZEOF(float)
AC_CHECK_SIZEOF(double)
AC_CHECK_SIZEOF(long double)
AC_CHECK_SIZEOF(pid_t)
AC_CHECK_SIZEOF(wchar_t)

AC_DEFINE_UNQUOTED(ASE_SIZEOF_CHAR,${ac_cv_sizeof_char})
AC_DEFINE_UNQUOTED(ASE_SIZEOF_SHORT,${ac_cv_sizeof_short})
AC_DEFINE_UNQUOTED(ASE_SIZEOF_INT,${ac_cv_sizeof_int})
AC_DEFINE_UNQUOTED(ASE_SIZEOF_LONG,${ac_cv_sizeof_long})
AC_DEFINE_UNQUOTED(ASE_SIZEOF_LONG_LONG,${ac_cv_sizeof_long_long})
AC_DEFINE_UNQUOTED(ASE_SIZEOF___INT8,${ac_cv_sizeof___int8})
AC_DEFINE_UNQUOTED(ASE_SIZEOF___INT16,${ac_cv_sizeof___int16})
AC_DEFINE_UNQUOTED(ASE_SIZEOF___INT32,${ac_cv_sizeof___int32})
AC_DEFINE_UNQUOTED(ASE_SIZEOF___INT64,${ac_cv_sizeof___int64})
AC_DEFINE_UNQUOTED(ASE_SIZEOF___INT96,${ac_cv_sizeof___int96})
AC_DEFINE_UNQUOTED(ASE_SIZEOF___INT128,${ac_cv_sizeof___int128})
AC_DEFINE_UNQUOTED(ASE_SIZEOF_VOID_P,${ac_cv_sizeof_void_p})
AC_DEFINE_UNQUOTED(ASE_SIZEOF_FLOAT,${ac_cv_sizeof_float})
AC_DEFINE_UNQUOTED(ASE_SIZEOF_DOUBLE,${ac_cv_sizeof_double})
AC_DEFINE_UNQUOTED(ASE_SIZEOF_LONG_DOUBLE,${ac_cv_sizeof_long_double})
AC_DEFINE_UNQUOTED(ASE_SIZEOF_PID_T,${ac_cv_sizeof_pid_t})
AC_DEFINE_UNQUOTED(ASE_SIZEOF_WCHAR_T,${ac_cv_sizeof_wchar_t})

#AC_CHECK_FUNC(nanosleep,[AC_DEFINE([ASE_HAVE_NANOSLEEP])],
#	[AC_CHECK_LIB(rt,nanosleep, LIBS="${LIBS} -lrt", 	 
#		[AC_CHECK_LIB(posix4,nanosleep, LIBS="${LIBS} -lposix4")])]) 	 

# Checks for library functions.
#AC_FUNC_MEMCMP
#AC_FUNC_REALLOC
#AC_CHECK_FUNCS([memset])

AC_ARG_ENABLE([wchar], [AC_HELP_STRING([--enable-wchar],
	[use wchar_t a default charater type when enabled (default. yes)])],
	enable_wchar_is=$enableval,enable_wchar_is=yes)
if test "$enable_wchar_is" = "yes"
then
	AC_DEFINE([ASE_CHAR_IS_WCHAR],[],[char is wchar])
else
	AC_DEFINE([ASE_CHAR_IS_MCHAR],[],[char is mchar])
fi

AC_ARG_ENABLE([reentrant], [AC_HELP_STRING([--enable-reentrant],
	[define _REENTRANT (default. yes)])],
	enable_reentrant_is=$enableval,enable_reentrant_is=yes)
if test "$enable_reentrant_is" = "yes"
then
	[CFLAGS="$CFLAGS -D_REENTRANT -D_THREAD_SAFE"]
fi

AC_CONFIG_FILES([makefile awk/makefile lsp/makefile stx/makefile test/awk/makefile test/lsp/makefile test/stx/makefile])
AC_OUTPUT
