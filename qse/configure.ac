
dnl AC_PREREQ([2.67])

AC_INIT([qse],[0.5.6],[Chung, Hyung-Hwan (hyunghwan.chung@gmail.com)],[],[http://code.abiyo.net/@qse])
AC_CONFIG_HEADER([include/qse/config.h])
AC_CONFIG_AUX_DIR([ac])
AC_CONFIG_MACRO_DIR([m4])

dnl use CC and CPP for compilation tests with .c test programs.
AC_LANG(C)

dnl initialize automake
AM_INIT_AUTOMAKE([-Wall -Werror foreign])

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
AC_CHECK_HEADERS([stddef.h wchar.h wctype.h errno.h signal.h])
AC_CHECK_HEADERS([time.h sys/time.h utime.h spawn.h])
AC_CHECK_HEADERS([sys/resource.h sys/syscall.h sys/sendfile.h])
AC_CHECK_HEADERS([execinfo.h])

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
AC_CHECK_FUNCS([lseek64 stat64 fstat64 lstat64 ftruncate64])
AC_CHECK_FUNCS([timegm timelocal])
AC_CHECK_FUNCS([utime utimes])
AC_CHECK_FUNCS([sysconf])
AC_CHECK_FUNCS([backtrace backtrace_symbols])
AC_CHECK_FUNCS([fdopendir])
AC_CHECK_FUNCS([posix_spawn])

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
AC_SUBST(SOCKET_LIBS)

dnl check if sendfile and its variants exist in the standard c library
dnl it inspects the existence of each missing function in the sendfile library.
AC_CHECK_FUNCS([sendfile sendfile64 sendfilev sendfilev64])
if test "$ac_cv_func_sendfile" = "no"
then
	AC_CHECK_LIB([sendfile], [sendfile], [
		SENDFILE_LIBS="-lsendfile"
		AC_DEFINE(HAVE_SENDFILE, 1)
	])
fi
if test "$ac_cv_func_sendfile64" = "no"
then
	AC_CHECK_LIB([sendfile], [sendfile64], [
		SENDFILE_LIBS="-lsendfile"
		AC_DEFINE(HAVE_SENDFILE64, 1)
	])
fi
if test "$ac_cv_func_sendfilev" = "no"
then
	AC_CHECK_LIB([sendfile], [sendfilev], [
		SENDFILE_LIBS="-lsendfile"
		AC_DEFINE(HAVE_SENDFILEV, 1)
	])
fi
if test "$ac_cv_func_sendfilev64" = "no"
then
	AC_CHECK_LIB([sendfile], [sendfilev64], [
		SENDFILE_LIBS="-lsendfile"
		AC_DEFINE(HAVE_SENDFILEV64, 1)
	])
fi
AC_SUBST(SENDFILE_LIBS)

AC_STRUCT_DIRENT_D_TYPE
AC_CHECK_MEMBERS([struct stat.st_birthtime])
AC_CHECK_MEMBERS([struct stat.st_mtim.tv_nsec])
AC_CHECK_MEMBERS([struct stat.st_mtimespec.tv_nsec])

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

AC_DEFINE_UNQUOTED(QSE_PACKAGE_VERSION, "${VERSION}", [Package version])
AC_DEFINE_UNQUOTED(QSE_PACKAGE_VERSION_MAJOR, $(echo ${VERSION} | cut -d. -f1), [Major version number])
AC_DEFINE_UNQUOTED(QSE_PACKAGE_VERSION_MINOR, $(echo ${VERSION} | cut -d. -f2), [Minor version number])
AC_DEFINE_UNQUOTED(QSE_PACKAGE_VERSION_PATCH, $(echo ${VERSION} | cut -d. -f3), [Patch level])

AC_DEFINE_UNQUOTED(QSE_PACKAGE_AUTHOR,"${PACKAGE_BUGREPORT}", [Author])
AC_DEFINE_UNQUOTED(QSE_PACKAGE_NAME,"${PACKAGE_NAME}", [package name])
AC_DEFINE_UNQUOTED(QSE_PACKAGE_URL, "${PACKAGE_URL}", [Project URL])

AC_C_BIGENDIAN(
	[AC_DEFINE([QSE_ENDIAN_BIG],[],[Big Endian])],
	[AC_DEFINE([QSE_ENDIAN_LITTLE],[],[Little Endian])],
	[AC_DEFINE([QSE_ENDIAN_UNKNOWN],[],[Unknown Endian])])

dnl define extra options
AC_ARG_ENABLE([wchar], [AS_HELP_STRING([--enable-wchar],[use wchar_t a default charater type when enabled (default. yes)])],
	enable_wchar_is=$enableval,enable_wchar_is=yes)
test "${ac_cv_sizeof_wchar_t}" = "0" && enable_wchar_is=no
if test "${enable_wchar_is}" = "yes"
then
	AC_DEFINE([QSE_CHAR_IS_WCHAR],[],[char is wchar])
	AC_SUBST(CHAR_MODE, "QSE_CHAR_IS_WCHAR")
else
	AC_DEFINE([QSE_CHAR_IS_MCHAR],[],[char is mchar])
	AC_SUBST(CHAR_MODE, "QSE_CHAR_IS_MCHAR")
fi

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
	[CFLAGS="$CFLAGS -g -D_DEBUG -UNDEBUG -DQSE_BUILD_DEBUG"]
	[CXXFLAGS="$CXXFLAGS -g -D_DEBUG -UNDEBUG -DQSE_BUILD_DEBUG"]
	AC_SUBST(BUILD_MODE, "debug")
else
	[CFLAGS="$CFLAGS -DNDEBUG -U_DEBUG -DQSE_BUILD_RELEASE"]
	[CXXFLAGS="$CXXFLAGS -DNDEBUG -U_DEBUG -DQSE_BUILD_RELEASE"]
	AC_SUBST(BUILD_MODE, "release")
fi

AC_ARG_ENABLE([cxx], [AS_HELP_STRING([--enable-cxx],[build the library for C++ if a C++ compiler is available (default. yes)])],
	enable_cxx_is=$enableval,enable_cxx_is=yes) 
[test "${HAVE_CXX}" = "yes" || enable_cxx_is="no"]
AM_CONDITIONAL(ENABLE_CXX, test "${enable_cxx_is}" = "yes" )

AC_ARG_ENABLE([reentrant], [AS_HELP_STRING([--enable-reentrant],[define _REENTRANT (default. yes)])],
	enable_reentrant_is=$enableval,enable_reentrant_is=yes)
if test "$enable_reentrant_is" = "yes"
then
	AX_PTHREAD()
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

AC_SUBST(QSE_PROJECT_AUTHOR, "${PACKAGE_BUGREPORT}")
AC_SUBST(QSE_PROJECT_URL, "${PACKAGE_URL}")

AC_CONFIG_FILES([
	Makefile 
	README
	include/Makefile
	include/qse/Makefile
	include/qse/cmn/Makefile
	include/qse/awk/Makefile
	include/qse/cut/Makefile
	include/qse/sed/Makefile
	include/qse/stx/Makefile
	include/qse/net/Makefile
	lib/Makefile 
	lib/cmn/Makefile 
	lib/awk/Makefile 
	lib/cut/Makefile 
	lib/sed/Makefile 
	lib/stx/Makefile 
	lib/net/Makefile 
	cmd/Makefile 
	cmd/awk/Makefile 
	cmd/cut/Makefile 
	cmd/fs/Makefile 
	cmd/sed/Makefile 
	cmd/stx/Makefile 
	samples/Makefile 
	samples/cmn/Makefile 
	samples/awk/Makefile 
	samples/cut/Makefile 
	samples/sed/Makefile 
	samples/net/Makefile
	regress/Makefile
	regress/awk/Makefile
	regress/awk/regress.sh
	regress/sed/Makefile
	regress/sed/regress.sh
	doc/Makefile
	doc/page/Makefile
	doc/image/Makefile
	doc/Doxyfile
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
echo "-------------------------------------------------------------------------"
]
