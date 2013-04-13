dnl AC_PREREQ([2.67])

dnl Make sure you change the version information
dnl in include/qse/conf_*.h whenever you change the version 
dnl here. Those files don't depend on autoconf, thus requiring
dnl manual change.
AC_INIT([qse],[0.6.0],[Chung, Hyung-Hwan (hyunghwan.chung@gmail.com)],[],[http://code.abiyo.net/@qse])

AC_CONFIG_HEADER([include/qse/config.h])
AC_CONFIG_AUX_DIR([ac])
AC_CONFIG_MACRO_DIR([m4])

dnl use CC and CPP for compilation tests with .c test programs.
AC_LANG(C)

dnl initialize automake
AM_INIT_AUTOMAKE([-Wall -Werror foreign])

dnl enable silent rules if automake supports them.
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([no])])
dnl m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

dnl determine a C compiler to use
AC_PROG_CC

dnl determine a C++ compiler to use
AC_PROG_CXX

dnl check if the C++ compiler exists in PATH. 
AC_CHECK_PROG(HAVE_CXX, $CXX, yes, no)

dnl determine some key programs
AC_PROG_INSTALL
AC_PROG_EGREP
AC_PROG_LN_S

dnl check other tools
AC_CHECK_TOOL(RM, rm)
AC_CHECK_TOOL(RMDIR, rmdir)
AC_CHECK_TOOL(TRUE, true)
AC_CHECK_TOOL(ECHO, echo)

dnl avoid "'/bin/rm: cannot remove `libtoolT': No such file or directory"
RM="$RM -f"

dnl initialize libtool
LT_INIT
AC_SUBST(LIBTOOL_DEPS)

dnl 'recursive' type seems to have problems in setting LT_CONFIG_H
dnl so i use the classic 'subproject' type here
LT_CONFIG_LTDL_DIR([libltdl])

save_CFLAGS="$CFLAGS"
save_LDFLAGS="$LDFLAGS"
save_LIBS="$LIBS"
LTDL_INIT
LDFLAGS="$save_LDFLAGS"
CFLAGS="$save_CFLAGS"
LIBS="$save_LIBS"

dnl The lt_dladvise_init symbol was added with libtool-2.2
if test "x$with_included_ltdl" != "xyes"; then
	save_CFLAGS="$CFLAGS"
	save_LDFLAGS="$LDFLAGS"
	save_LIBS="$LIBS"
	CFLAGS="$CFLAGS $LTDLINCL"
	LDFLAGS="$LDFLAGS $LIBLTDL"
	AC_CHECK_LIB([ltdl], [lt_dladvise_init], [],
	             [AC_MSG_ERROR([installed libltdl is too old. install a newer libltdl or specify --with-included-ltdl])])
	LDFLAGS="$save_LDFLAGS"
	CFLAGS="$save_CFLAGS"
	LIBS="$save_LIBS"
fi

dnl overrides the default CFLAGS setting
if test "$ac_test_CFLAGS" != "set"
then
	if test "$GCC" = "yes"
	then
		CFLAGS="$CFLAGS -Wall"
	fi
fi

if test "$ac_test_CXXFLAGS" != "set"
then
	if test "$GCC" = "yes"
	then
		CXXFLAGS="$CXXFLAGS -Wall"
	fi
fi

dnl indicate the existence of config.h
CFLAGS="$CFLAGS -DQSE_HAVE_CONFIG_H"
CXXFLAGS="$CXXFLAGS -DQSE_HAVE_CONFIG_H"

dnl make visible the 64-bit interface to the file system
CFLAGS="$CFLAGS -D_LARGEFILE64_SOURCE"
CXXFLAGS="$CXXFLAGS -D_LARGEFILE64_SOURCE"

dnl define the WIN32 conditional if necessary
case "$host" in
*-*-mingw*|*-*-cygwin*)
	#CFLAGS="$CFLAGS -D_WIN32_WINNT=0x0400"
	platform_win32=yes
esac
AM_CONDITIONAL(WIN32, test "${platform_win32}" = "yes" )

dnl check the math library (check if -lm is needed)
LT_LIB_M
AC_SUBST(LIBM, $LIBM)

dnl check header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([stddef.h wchar.h wctype.h errno.h signal.h fcntl.h dirent.h])
AC_CHECK_HEADERS([time.h sys/time.h utime.h spawn.h execinfo.h ucontext.h])
AC_CHECK_HEADERS([sys/resource.h sys/wait.h sys/syscall.h sys/ioctl.h])
AC_CHECK_HEADERS([sys/sendfile.h sys/epoll.h sys/event.h])
AC_CHECK_HEADERS([sys/sysctl.h sys/socket.h sys/sockio.h ifaddrs.h])
AC_CHECK_HEADERS([net/if.h net/if_dl.h], [], [], [
	#include <sys/types.h>
	#include <sys/socket.h>])
AC_CHECK_HEADERS([sys/stropts.h sys/macstat.h linux/ethtool.h linux/sockios.h])
AC_CHECK_HEADERS([crt_externs.h sys/prctl.h uci.h])

dnl check data types
AC_CHECK_TYPE([wchar_t], 
	[AC_DEFINE([HAVE_WCHAR_T_IN_STDDEF_H],
	           [],[wchar_t is available in stddef.h])],
	[],
	[#include <stddef.h>])

dnl check functions
AC_CHECK_FUNCS([uselocale])
AC_CHECK_FUNCS([mbrlen mbrtowc wcrtomb])
AC_CHECK_FUNCS([mbsnrtowcs mbsrtowcs wcsnrtombs wcsrtombs])
AC_CHECK_FUNCS([wctype iswctype wctrans towctrans])
AC_CHECK_FUNCS([isblank iswblank])
AC_CHECK_FUNCS([lseek64 stat64 fstat64 lstat64 ftruncate64 readdir64])
AC_CHECK_FUNCS([timegm timelocal])
AC_CHECK_FUNCS([utime utimes])
AC_CHECK_FUNCS([sysconf prctl fdopendir setrlimit getrlimit])
AC_CHECK_FUNCS([backtrace backtrace_symbols])
AC_CHECK_FUNCS([fork vfork posix_spawn gettid nanosleep select])
AC_CHECK_FUNCS([makecontext swapcontext getcontext setcontext])
AC_CHECK_FUNCS([_vsnprintf _vsnwprintf])

OLDLIBS="$LIBS"
LIBS="$LIBM $LIBS"
AC_CHECK_FUNCS([powl fmodl sinl cosl tanl atanl atan2l logl log10l expl sqrtl ceill floorl roundl])
AC_CHECK_FUNCS([pow fmod sin cos tan atan atan2 log log10 exp sqrt ceil floor round])
AC_CHECK_FUNCS([powf fmodf sinf cosf tanf atanf atan2f logf log10f expf sqrtf ceilf floorf roundf])
LIBS="$OLDLIBS"

dnl OLDLIBS="$LIBS"
dnl AC_SEARCH_LIBS([connect], [socket])
dnl LIBS="$OLDLIBS"
dnl if test "$ac_cv_search_connect" != "none required" -a "$ac_cv_search_connect" != "no"
dnl then
dnl	SOCKET_LIBS="$SOCKET_LIBS -lsocket"
dnl fi
dnl AC_SUBST(SOCKET_LIBS)
AC_CHECK_FUNCS([connect gethostbyname])
if test "$ac_cv_func_gethostbyname" = "no"
then
	AC_CHECK_LIB([nsl], [gethostbyname], [
		SOCKET_LIBS="$SOCKET_LIBS -lnsl"
		AC_DEFINE(HAVE_GETHOSTBYNAME, 1)
	])
fi
if test "$ac_cv_func_connect" = "no"
then
	AC_CHECK_LIB([socket], [connect], [
		SOCKET_LIBS="$SOCKET_LIBS -lsocket"
		AC_DEFINE(HAVE_CONNECT, 1)
	])
fi
if test "${platform_win32}" = "yes"
then
	SOCKET_LIBS="$SOCKET_LIBS -lws2_32"
fi
AC_SUBST(SOCKET_LIBS)

OLDLIBS="$LIBS"
LIBS="$SOCKET_LIBS $LIBS"
AC_CHECK_FUNCS([if_nametoindex if_indextoname getifaddrs])
LIBS="$OLDLIBS"

dnl check if sendfile and its variants exist in the standard c library
dnl it inspects the existence of each missing function in the sendfile library.
AC_CHECK_FUNCS([sendfile sendfile64 sendfilev sendfilev64])
if test "$ac_cv_func_sendfile" = "no"
then
	AC_CHECK_LIB([sendfile], [sendfile], [
		SENDFILE_LIBS="-lsendfile"
		dnl only for consistency with AC_CHECK_FUNCS
		AC_DEFINE(HAVE_SENDFILE, 1)
	])
fi
if test "$ac_cv_func_sendfile64" = "no"
then
	AC_CHECK_LIB([sendfile], [sendfile64], [
		SENDFILE_LIBS="-lsendfile"
		dnl only for consistency with AC_CHECK_FUNCS
		AC_DEFINE(HAVE_SENDFILE64, 1)
	])
fi
if test "$ac_cv_func_sendfilev" = "no"
then
	AC_CHECK_LIB([sendfile], [sendfilev], [
		SENDFILE_LIBS="-lsendfile"
		dnl only for consistency with AC_CHECK_FUNCS
		AC_DEFINE(HAVE_SENDFILEV, 1)
	])
fi
if test "$ac_cv_func_sendfilev64" = "no"
then
	AC_CHECK_LIB([sendfile], [sendfilev64], [
		SENDFILE_LIBS="-lsendfile"
		dnl only for consistency with AC_CHECK_FUNCS
		AC_DEFINE(HAVE_SENDFILEV64, 1)
	])
fi
AC_SUBST(SENDFILE_LIBS)

AC_CHECK_FUNCS([epoll_create epoll_create1])
if test "$ac_cv_func_epoll_create" = "yes"
then
	AC_DEFINE(HAVE_EPOLL, 1, [epoll support])
fi

AC_CHECK_FUNCS([kqueue kqueue1 kevent])

dnl check is the import library for unicows.dll exists
dnl this check doesn't look for a particular symbol
dnl but for the symbol 'main' since i want to check 
dnl the existence of the libaray.
AC_CHECK_LIB([unicows], [main], [UNICOWS_LIBS="-lunicows"])
AC_SUBST(UNICOWS_LIBS)

AC_ARG_ENABLE([ssl], [AS_HELP_STRING([--enable-ssl],[build the library in the ssl mode (default. yes)])],
	enable_ssl_is=$enableval,enable_ssl_is=yes)
if test "$enable_ssl_is" = "yes"
then
	dnl check for an SSL library
	AC_CHECK_FUNCS([SSL_library_init])
	if test "$ac_cv_func_SSL_library_init" = "yes"
	then
		AC_DEFINE(HAVE_SSL, 1, [ssl support])
	else
		AC_CHECK_LIB([ssl], [SSL_library_init], [
			SSL_LIBS="-lssl"
			AC_DEFINE(HAVE_SSL, 1, [ssl support])

			dnl additional check
			AC_CHECK_LIB([crypto], [ERR_get_error], [
				SSL_LIBS="${SSL_LIBS} -lcrypto"
			])

			dnl only for consistency with AC_CHECK_FUNCS
			AC_DEFINE(HAVE_SSL_LIBRARY_INIT, 1)
		])
	fi
fi
AC_SUBST(SSL_LIBS)

dnl MPI (optional)
LX_FIND_MPI(C)
AM_CONDITIONAL(HAVE_C_MPI, test "${have_C_mpi}" = "yes" )
dnl AM_CONDITIONAL(HAVE_CXX_MPI, test "${have_CXX_mpi}" = "yes" )

dnl libuci (optional)
AC_CHECK_LIB([uci], [uci_alloc_context], [UCI_LIBS="-luci"])
AC_SUBST(UCI_LIBS)
AM_CONDITIONAL(HAVE_LIBUCI, test "${ac_cv_lib_uci_uci_alloc_context}" = "yes" -a "${ac_cv_header_uci_h}" = "yes")

AC_STRUCT_DIRENT_D_TYPE
AC_CHECK_MEMBERS([struct stat.st_birthtime])
AC_CHECK_MEMBERS([struct stat.st_mtim.tv_nsec])
AC_CHECK_MEMBERS([struct stat.st_birthtim.tv_nsec])
AC_CHECK_MEMBERS([struct stat.st_mtimespec.tv_nsec])
AC_CHECK_MEMBERS([struct stat.st_birthtimespec.tv_nsec])

AC_CHECK_MEMBERS([struct ifreq.ifr_ifindex, struct ifreq.ifr_mtu], [], [],
	[#include <sys/types.h>
	 #include <sys/socket.h>
	 #ifdef HAVE_NET_IF_H
	 #include <net/if.h>
	 #endif])

AC_CHECK_TYPES([struct lifconf, struct lifreq, struct if_laddrreq], [], [],
	[#include <sys/types.h>
	 #include <sys/socket.h>
	 #ifdef HAVE_NET_IF_H
	 #include <net/if.h>
	 #endif])

dnl check the size of primitive data types
AC_CHECK_SIZEOF(char,,[[]])
AC_CHECK_SIZEOF(short,,[[]])
AC_CHECK_SIZEOF(int,,[[]])
AC_CHECK_SIZEOF(long,,[[]])
AC_CHECK_SIZEOF(long long,,[[]])
AC_CHECK_SIZEOF(__int8,,[[]])
AC_CHECK_SIZEOF(__int16,,[[]])
AC_CHECK_SIZEOF(__int32,,[[]])
AC_CHECK_SIZEOF(__int64,,[[]])
AC_CHECK_SIZEOF(__int128,,[[]])
AC_CHECK_SIZEOF(__int8_t,,[[]])
AC_CHECK_SIZEOF(__int16_t,,[[]])
AC_CHECK_SIZEOF(__int32_t,,[[]])
AC_CHECK_SIZEOF(__int64_t,,[[]])
AC_CHECK_SIZEOF(__int128_t,,[[]])
dnl AC_CHECK_SIZEOF doesn't work without white-space between void and *
AC_CHECK_SIZEOF(void *,,[[]])
AC_CHECK_SIZEOF(float,,[[]])
AC_CHECK_SIZEOF(double,,[[]])
AC_CHECK_SIZEOF(long double,,[[]])
AC_CHECK_SIZEOF(__float128,,[[]])

AC_CHECK_SIZEOF(wchar_t)
AC_CHECK_SIZEOF(off_t)
AC_CHECK_SIZEOF(off64_t)

AC_CHECK_SIZEOF(mbstate_t,,[#include <wchar.h>])
AX_CHECK_NUMVALOF(MB_LEN_MAX,[32],[#include <limits.h>])

if test "${platform_win32}" == "yes"
then
AC_CHECK_SIZEOF(struct sockaddr_in,,[
	#include <winsock2.h>
	#include <ws2tcpip.h>])
AC_CHECK_SIZEOF(struct sockaddr_in6,, [
	#include <winsock2.h>
	#include <ws2tcpip.h>])
else
AC_CHECK_SIZEOF(struct sockaddr_in,,[
	#include <sys/socket.h>
	#include <netinet/in.h>])
AC_CHECK_SIZEOF(struct sockaddr_in6,, [
	#include <sys/socket.h>
	#include <netinet/in.h>])
fi

dnl gcc 3.4.3 on opensolaris x86 gave this warning without -msse or 
dnl something similar.
dnl    SSE vector argument without SSE enabled changes the ABI
dnl it also ended with an internal compiler error while compiling 
dnl some expressions like 'v % base' where v is of the __int128_t type.
dnl so i've added this check to determine if the compiler has
dnl proper support for __int128_t.
dnl 
dnl Changing ac_cv_sizeof___int128_t here doesn't corret SIZEOF___INT128_T
dnl since it's already generated. It only affects QSE_SIZEOF___INT128_T below.
if test ${ac_cv_sizeof___int128_t} -gt 0
then
	AC_MSG_CHECKING([__int128_t with %])
	AC_COMPILE_IFELSE(
		[AC_LANG_PROGRAM([], [
			volatile __int128_t x, base;
			x = (__int128_t)&base;
			base = (__int128_t)&x;
			x = x % base;
			return (int)x;
		])],

		[
			AC_MSG_RESULT(yes)
		],
		[
			AC_MSG_RESULT(no)
			ac_cv_sizeof___int128_t=0
		]
	)
fi

AC_DEFINE_UNQUOTED(QSE_SIZEOF_CHAR, ${ac_cv_sizeof_char}, [sizeof(char)])
AC_DEFINE_UNQUOTED(QSE_SIZEOF_SHORT, ${ac_cv_sizeof_short}, [sizeof(short)])
AC_DEFINE_UNQUOTED(QSE_SIZEOF_INT, ${ac_cv_sizeof_int}, [sizeof(int)])
AC_DEFINE_UNQUOTED(QSE_SIZEOF_LONG, ${ac_cv_sizeof_long}, [sizeof(long)])
AC_DEFINE_UNQUOTED(QSE_SIZEOF_LONG_LONG, ${ac_cv_sizeof_long_long}, [sizeof(long long)])
AC_DEFINE_UNQUOTED(QSE_SIZEOF___INT8, ${ac_cv_sizeof___int8}, [sizeof(__int8)])
AC_DEFINE_UNQUOTED(QSE_SIZEOF___INT16, ${ac_cv_sizeof___int16}, [sizeof(__int16)])
AC_DEFINE_UNQUOTED(QSE_SIZEOF___INT32, ${ac_cv_sizeof___int32}, [sizeof(__int32)])
AC_DEFINE_UNQUOTED(QSE_SIZEOF___INT64, ${ac_cv_sizeof___int64}, [sizeof(__int64)])
AC_DEFINE_UNQUOTED(QSE_SIZEOF___INT128, ${ac_cv_sizeof___int128}, [sizeof(__int128)])
AC_DEFINE_UNQUOTED(QSE_SIZEOF___INT128_T, ${ac_cv_sizeof___int128_t}, [sizeof(__int128_t)])
AC_DEFINE_UNQUOTED(QSE_SIZEOF_VOID_P, ${ac_cv_sizeof_void_p}, [sizeof(void*)])
AC_DEFINE_UNQUOTED(QSE_SIZEOF_FLOAT, ${ac_cv_sizeof_float}, [sizeof(float)])
AC_DEFINE_UNQUOTED(QSE_SIZEOF_DOUBLE, ${ac_cv_sizeof_double}, [sizeof(double)])
AC_DEFINE_UNQUOTED(QSE_SIZEOF_LONG_DOUBLE, ${ac_cv_sizeof_long_double}, [sizeof(long double)])
AC_DEFINE_UNQUOTED(QSE_SIZEOF___FLOAT128, ${ac_cv_sizeof___float128}, [sizeof(__float128)])

AC_DEFINE_UNQUOTED(QSE_SIZEOF_WCHAR_T, ${ac_cv_sizeof_wchar_t}, [sizeof(wchar_t)])
AC_DEFINE_UNQUOTED(QSE_SIZEOF_OFF_T, ${ac_cv_sizeof_off_t}, [sizeof(off_t)])
AC_DEFINE_UNQUOTED(QSE_SIZEOF_OFF64_T, ${ac_cv_sizeof_off64_t}, [sizeof(off64_t)])

AC_DEFINE_UNQUOTED(QSE_SIZEOF_MBSTATE_T, ${ac_cv_sizeof_mbstate_t}, [sizeof(mbstate_t)])
AC_DEFINE_UNQUOTED(QSE_MBLEN_MAX, ${ax_cv_numvalof_MB_LEN_MAX}, [MB_LEN_MAX])

AC_DEFINE_UNQUOTED(QSE_SIZEOF_STRUCT_SOCKADDR_IN, ${ac_cv_sizeof_struct_sockaddr_in}, [sizeof(struct sockaddr_in)])
AC_DEFINE_UNQUOTED(QSE_SIZEOF_STRUCT_SOCKADDR_IN6, ${ac_cv_sizeof_struct_sockaddr_in6}, [sizeof(struct sockaddr_in6)])

qse_package_version_major="`echo ${PACKAGE_VERSION} | cut -d. -f1`"
qse_package_version_minor="`echo ${PACKAGE_VERSION} | cut -d. -f2`"
qse_package_version_patch="`echo ${PACKAGE_VERSION} | cut -d. -f3`"
AC_SUBST(PACKAGE_VERSION_MAJOR, ${qse_package_version_major})
AC_SUBST(PACKAGE_VERSION_MINOR, ${qse_package_version_minor})
AC_SUBST(PACKAGE_VERSION_PATCH, ${qse_package_version_patch})

AC_DEFINE_UNQUOTED(QSE_PACKAGE_VERSION, "${PACKAGE_VERSION}", [Package version])
dnl AC_DEFINE_UNQUOTED(QSE_PACKAGE_VERSION_MAJOR, $(echo ${VERSION} | cut -d. -f1), [Major version number])
dnl AC_DEFINE_UNQUOTED(QSE_PACKAGE_VERSION_MINOR, $(echo ${VERSION} | cut -d. -f2), [Minor version number])
dnl AC_DEFINE_UNQUOTED(QSE_PACKAGE_VERSION_PATCH, $(echo ${VERSION} | cut -d. -f3), [Patch level])
AC_DEFINE_UNQUOTED(QSE_PACKAGE_VERSION_MAJOR, ${qse_package_version_major}, [Major version number])
AC_DEFINE_UNQUOTED(QSE_PACKAGE_VERSION_MINOR, ${qse_package_version_minor}, [Minor version number])
AC_DEFINE_UNQUOTED(QSE_PACKAGE_VERSION_PATCH, ${qse_package_version_patch}, [Patch level])

AC_DEFINE_UNQUOTED(QSE_PACKAGE_AUTHOR,"${PACKAGE_BUGREPORT}", [Author])
AC_DEFINE_UNQUOTED(QSE_PACKAGE_NAME,"${PACKAGE_NAME}", [package name])
AC_DEFINE_UNQUOTED(QSE_PACKAGE_URL, "${PACKAGE_URL}", [Project URL])

AC_C_BIGENDIAN(
	[AC_DEFINE([QSE_ENDIAN_BIG],[],[Big Endian])],
	[AC_DEFINE([QSE_ENDIAN_LITTLE],[],[Little Endian])],
	[AC_DEFINE([QSE_ENDIAN_UNKNOWN],[],[Unknown Endian])])

dnl define extra options
AC_ARG_ENABLE([wchar], [AS_HELP_STRING([--enable-wchar],[Use the wide character type as the default charater type when enabled (default. yes)])],
	enable_wchar_is=$enableval,enable_wchar_is=yes)
test "${ac_cv_sizeof_wchar_t}" = "0" && enable_wchar_is=no
if test "${enable_wchar_is}" = "yes"
then
	AC_DEFINE([QSE_CHAR_IS_WCHAR],[],[The default character type is the wide character type])
	AC_SUBST(CHAR_MODE, "QSE_CHAR_IS_WCHAR")
else
	AC_DEFINE([QSE_CHAR_IS_MCHAR],[],[The default character type is the multi-byte character type])
	AC_SUBST(CHAR_MODE, "QSE_CHAR_IS_MCHAR")
fi
AM_CONDITIONAL(WCHAR, test "${enable_wchar_is}" = "yes")

AC_ARG_ENABLE([bundled-unicode], [AS_HELP_STRING([--enable-bundled-unicode],[use thn bundled unicode routines (default. no)])],
	enable_bundled_unicode_is=$enableval,enable_bundled_unicode_is=yes)
if test "${ac_cv_header_wctype_h}" = "no" -o "${ac_cv_func_wctrans}" = "no" -o "${ac_cv_func_wctype}" = "no"
then
	dnl force bundled-unicode to yes if some wchar functions are not available
	enable_bundled_unicode_is=yes
fi
if test "${enable_bundled_unicode_is}" = "yes"
then
	AC_DEFINE([QSE_ENABLE_BUNDLED_UNICODE],[],[use the bundled unicode routines])
fi
AM_CONDITIONAL(ENABLE_BUNDLED_UNICODE, test "${enable_bundled_unicode_is}" = "yes")

AC_ARG_ENABLE([xcmgrs], [AS_HELP_STRING([--enable-xcmgrs],[include more built-in cmgrs like cp949 and cp950 when enabled (default. yes)])],
	enable_xcmgrs_is=$enableval,enable_xcmgrs_is=yes)
if test "${enable_xcmgrs_is}" = "yes"
then
	AC_DEFINE([QSE_ENABLE_XCMGRS],[],[include more built-in cmgrs like cp949 and cp950])
fi
AM_CONDITIONAL(ENABLE_XCMGRS, test "${enable_xcmgrs_is}" = "yes" )

AC_ARG_ENABLE([syscall], [AS_HELP_STRING([--enable-syscall],[use the syscall() function to call system calls (default. no)])],
	enable_syscall_is=$enableval,enable_syscall_is=no)
if test "${enable_syscall_is}" = "yes"
then
	AC_DEFINE([QSE_USE_SYSCALL],[],[use the syscall() function to invoke a system call])
fi

AC_ARG_ENABLE([debug], [AS_HELP_STRING([--enable-debug],[build the library in the debug mode (default. no)])],
	enable_debug_is=$enableval,enable_debug_is=no)
if test "$enable_debug_is" = "yes"
then
	[CFLAGS="$CFLAGS -g -D_DEBUG -UNDEBUG -DQSE_BUILD_DEBUG -U_FORTIFY_SOURCE"]
	[CXXFLAGS="$CXXFLAGS -g -D_DEBUG -UNDEBUG -DQSE_BUILD_DEBUG -U_FORTIFY_SOURCE"]
	AC_SUBST(BUILD_MODE, "debug")
else
	[CFLAGS="$CFLAGS -DNDEBUG -U_DEBUG -DQSE_BUILD_RELEASE -U_FORTIFY_SOURCE"]
	[CXXFLAGS="$CXXFLAGS -DNDEBUG -U_DEBUG -DQSE_BUILD_RELEASE -U_FORTIFY_SOURCE"]
	AC_SUBST(BUILD_MODE, "release")
fi

AC_ARG_ENABLE([cxx], [AS_HELP_STRING([--enable-cxx],[build the library for C++ if a C++ compiler is available (default. yes)])],
	enable_cxx_is=$enableval,enable_cxx_is=yes) 
[test "${HAVE_CXX}" = "yes" || enable_cxx_is="no"]
AM_CONDITIONAL(ENABLE_CXX, test "${enable_cxx_is}" = "yes" )

AX_PTHREAD()
AC_ARG_ENABLE([reentrant], [AS_HELP_STRING([--enable-reentrant],[add thread options (default. no)])],
	enable_reentrant_is=$enableval,enable_reentrant_is=no)
if test "$enable_reentrant_is" = "yes"
then
	[CFLAGS="$CFLAGS $PTHREAD_CFLAGS"]
	[CXXFLAGS="$CXXFLAGS $PTHREAD_CFLAGS"]
fi

AC_ARG_ENABLE([sedtracer], [AS_HELP_STRING([--enable-sedtracer],[enable qse_sed_setexectracer() (default. yes)])],
	enable_sedtracer_is=$enableval,enable_sedtracer_is=yes)
if test "${enable_sedtracer_is}" = "yes"
then
	AC_DEFINE([QSE_ENABLE_SEDTRACER],[],[enable qse_sed_setexectracer()])
fi

AC_MSG_CHECKING([for socklen_t in sys/socket.h])
AC_TRY_COMPILE([#define _POSIX_PII_SOCKET
#include <sys/types.h>
#include <sys/socket.h>], [socklen_t x = 5;],
	[AC_DEFINE(HAVE_SOCKLEN_T, 1, [Define it socklen_t typedef is in sys/socket.h.])
	AC_MSG_RESULT(yes)],
	[AC_MSG_RESULT(no)]
)

if test "${enable_shared}" = "no" -a "${enable_static}" = "yes"
then
	AC_DEFINE([QSE_ENABLE_STATIC_MODULE],[],[link modules statically into the main library])
fi
AM_CONDITIONAL(ENABLE_STATIC_MODULE, test "${enable_shared}" = "no" -a "${enable_static}" = "yes")

AC_SUBST(QSE_SIZEOF_WCHAR_T, $ac_cv_sizeof_wchar_t)
AC_SUBST(QSE_SIZEOF_LONG_LONG, $ac_cv_sizeof_long_long)
AC_SUBST(QSE_SIZEOF_LONG, $ac_cv_sizeof_long)
AC_SUBST(QSE_SIZEOF_INT, $ac_cv_sizeof_int)
AC_SUBST(QSE_SIZEOF_SHORT, $ac_cv_sizeof_short)
AC_SUBST(QSE_SIZEOF_CHAR, $ac_cv_sizeof_char)
AC_SUBST(QSE_SIZEOF_VOID_P, $ac_cv_sizeof_void_p)
AC_SUBST(QSE_SIZEOF_FLOAT, $ac_cv_sizeof_float)
AC_SUBST(QSE_SIZEOF_DOUBLE, $ac_cv_sizeof_double)
AC_SUBST(QSE_SIZEOF_LONG_DOUBLE, $ac_cv_sizeof_long_double)
AC_SUBST(QSE_SIZEOF_OFF_T, $ac_cv_sizeof_off_t)
AC_SUBST(QSE_SIZEOF_OFF64_T, $ac_cv_sizeof_off64_t)

AC_SUBST(QSE_PROJECT_AUTHOR, "${PACKAGE_BUGREPORT}")
AC_SUBST(QSE_PROJECT_URL, "${PACKAGE_URL}")

AC_CONFIG_FILES([
	Makefile 
	README
	include/Makefile
	include/qse/Makefile
	include/qse/cmn/Makefile
	include/qse/awk/Makefile
	include/qse/sed/Makefile
	include/qse/xli/Makefile
	include/qse/http/Makefile
	lib/Makefile 
	lib/cmn/Makefile 
	lib/awk/Makefile 
	lib/sed/Makefile 
	lib/xli/Makefile 
	lib/http/Makefile 
	cmd/Makefile 
	cmd/awk/Makefile 
	cmd/sed/Makefile 
	cmd/xli/Makefile 
	cmd/http/Makefile 
	samples/Makefile 
	samples/cmn/Makefile 
	samples/awk/Makefile 
	samples/sed/Makefile 
	samples/http/Makefile
	regress/Makefile
	regress/awk/Makefile
	regress/awk/regress.sh
	regress/sed/Makefile
	regress/sed/regress.sh
	doc/Makefile
	doc/Doxyfile
	tools/Makefile
])
AC_OUTPUT

[
echo 
echo "-[SUMMARY]---------------------------------------------------------------"
echo "Configured for ${host}"
echo "  Build mode : ${BUILD_MODE}"
echo "  Source directory: ${srcdir}"
echo "  Installation directory: ${prefix}"
echo "  C compiler: ${CC} ${CFLAGS}"
echo "  C++ compiler: ${CXX} ${CXXFLAGS}"
echo "  C++ support: ${enable_cxx_is}"
echo "  Wide character: ${enable_wchar_is}" 
echo "  Math library: ${LIBM}"
echo "  Pthread library: ${PTHREAD_LIBS}"
echo "  Socket library: ${SOCKET_LIBS}"
echo "  Sendfile library: ${SENDFILE_LIBS}"
echo "  SSL library: ${SSL_LIBS}"
echo "-------------------------------------------------------------------------"
]
