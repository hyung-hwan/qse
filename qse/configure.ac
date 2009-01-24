dnl AC_PREREQ(2.59)
AC_INIT([qse],[0.5.0],[bacon@abiyo.net])
AC_CONFIG_HEADER([include/qse/config.h])
AC_CONFIG_AUX_DIR(autoconf)

dnl use CC and CPP for compilation tests with .c test programs.
AC_LANG(C)

dnl initialize automake
AM_INIT_AUTOMAKE([-Wall -Werror foreign])

dnl determine a C compiler to use
AC_PROG_CC

dnl determine a C++ compiler to use
AC_PROG_CXX

dnl determine an Objective-C compiler to use
AC_PROG_OBJC

dnl check if the Objective-C compiler exists in PATH. 
AC_CHECK_PROG(HAVE_OBJC, $OBJC, yes, no)

dnl check if the C++ compiler exists in PATH. 
AC_CHECK_PROG(HAVE_CXX, $CXX, yes, no)

dnl check other tools
AC_CHECK_TOOL(AR, ar)
AC_CHECK_TOOL(RANLIB, ranlib)
AC_CHECK_TOOL(STRIP, strip)
AC_CHECK_TOOL(OBJDUMP, objdump)
AC_CHECK_TOOL(NM, nm)
AC_CHECK_TOOL(LD, ld)
AC_CHECK_TOOL(GREP, grep)
AC_CHECK_TOOL(RM, rm)
AC_CHECK_TOOL(SED, sed)

dnl initialize libtool
AC_PROG_LIBTOOL
AC_SUBST(LIBTOOL_DEPS)

#dnl overrides the default CFLAGS setting
#if test "$ac_test_CFLAGS" = "set"
#then
#	CFLAGS=$ac_save_CFLAGS
#else
#	if test "$GCC" = "yes"
#	then
#		CFLAGS="-O2"
#	else
#		CFLAGS=
#	fi
#fi

#if test "$ac_test_CXXFLAGS" = "set"
#then
#	CXXFLAGS=$ac_save_CXXFLAGS
#else
#	if test "$GCC" = "yes"
#	then
#		CXXFLAGS="-O2"
#	else
#		CXXFLAGS=
#	fi
#fi

dnl make visible the 64-bit interface to the file system
CFLAGS="$CFLAGS -D_LARGEFILE64_SOURCE -DQSE_HAVE_CONFIG_H"
OBJCFLAGS="$OBJCFLAGS -D_LARGEFILE64_SOURCE -DQSE_HAVE_CONFIG_H"
CXXFLAGS="$CXXFLAGS -D_LARGEFILE64_SOURCE -DQSE_HAVE_CONFIG_H"

dnl define the WIN32 conditional if necessary
case "$host" in
*-*-mingw*|*-*-cygwin*)
	#CFLAGS="$CFLAGS -D_WIN32_WINNT=0x0400"
	platform_win32=yes
esac
AM_CONDITIONAL(WIN32, test "${platform_win32}" = "yes" )

dnl check the math library (check if -lm is needed)
AC_CHECK_LIBM
AC_SUBST(LIBM, $LIBM)

dnl check header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([stddef.h wchar.h wctype.h sys/syscall.h])

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
AC_CHECK_FUNCS([lseek64 stat64 fstat64 ftruncate64])
AC_CHECK_FUNCS([timegm timelocal])

OLDLIBS="$LIBS"
LIBS="$LIBM $LIBS"
AC_CHECK_FUNCS([powl sinl cosl tanl atanl atan2l logl expl sqrtl])
AC_CHECK_FUNCS([pow sin cos tan atan atan2 log exp sqrt])
AC_CHECK_FUNCS([powf sinf cosf tanf atanf atan2f logf expf sqrtf])
LIBS="$OLDLIBS"

dnl check the size of primitive data types
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
dnl AC_CHECK_SIZEOF doesn't work without white-space between void and *
AC_CHECK_SIZEOF(void *)
AC_CHECK_SIZEOF(float)
AC_CHECK_SIZEOF(double)
AC_CHECK_SIZEOF(long double)
AC_CHECK_SIZEOF(wchar_t)

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
AC_DEFINE_UNQUOTED(QSE_SIZEOF_VOID_P, ${ac_cv_sizeof_void_p}, [sizeof(void*)])
AC_DEFINE_UNQUOTED(QSE_SIZEOF_FLOAT, ${ac_cv_sizeof_float}, [sizeof(float)])
AC_DEFINE_UNQUOTED(QSE_SIZEOF_DOUBLE, ${ac_cv_sizeof_double}, [sizeof(double)])
AC_DEFINE_UNQUOTED(QSE_SIZEOF_LONG_DOUBLE, ${ac_cv_sizeof_long_double}, [sizeof(long double)])
AC_DEFINE_UNQUOTED(QSE_SIZEOF_WCHAR_T, ${ac_cv_sizeof_wchar_t}, [sizeof(wchar_t)])

AC_DEFINE_UNQUOTED(QSE_VERSION, "${VERSION}", [package version])
AC_DEFINE_UNQUOTED(QSE_VERSION_MAJOR, $(echo ${VERSION} | cut -d. -f1), [major version number])
AC_DEFINE_UNQUOTED(QSE_VERSION_MINOR, $(echo ${VERSION} | cut -d. -f2), [minor version number])
AC_DEFINE_UNQUOTED(QSE_VERSION_PATCH, $(echo ${VERSION} | cut -d. -f3), [patch level])
AC_DEFINE_UNQUOTED(QSE_AUTHOR,"${PACKAGE_BUGREPORT}", [author's email address])

AC_C_BIGENDIAN(
	[AC_DEFINE([QSE_ENDIAN_BIG],[],[Big Endian])],
	[AC_DEFINE([QSE_ENDIAN_LITTLE],[],[Little Endian])],
	[AC_DEFINE([QSE_ENDIAN_UNKNOWN],[],[Unknown Endian])])

dnl define extra options
AC_ARG_ENABLE([wchar], [AC_HELP_STRING([--enable-wchar],
	[use wchar_t a default charater type when enabled (default. yes)])],
	enable_wchar_is=$enableval,enable_wchar_is=yes)
test "${ac_cv_sizeof_wchar_t}" = "0" && enable_wchar_is=no
if test "${enable_wchar_is}" = "yes"
then
	AC_DEFINE([QSE_CHAR_IS_WCHAR],[],[char is wchar])
else
	AC_DEFINE([QSE_CHAR_IS_MCHAR],[],[char is mchar])
fi

AC_ARG_ENABLE([syscall], [AC_HELP_STRING([--enable-syscall],
	[use the syscall() function to call system calls (default. yes)])],
	enable_syscall_is=$enableval,enable_syscall_is=yes)
if test "${enable_syscall_is}" = "yes"
then
	AC_DEFINE([QSE_USE_SYSCALL],[],[use the syscall() function to invoke a system call])
fi

AC_ARG_ENABLE([debug], [AC_HELP_STRING([--enable-debug],
	[build the library in the debug mode (default. no)])],
	enable_debug_is=$enableval,enable_debug_is=no)
if test "$enable_debug_is" = "yes"
then
	[CFLAGS="$CFLAGS -g -D_DEBUG -UNDEBUG"]
	[OBJCFLAGS="$OBJCFLAGS -g -D_DEBUG -UNDEBUG"]
	[CXXFLAGS="$CXXFLAGS -g -D_DEBUG -UNDEBUG"]
	AC_SUBST(BUILD_MODE, "debug")
else
	[CFLAGS="$CFLAGS -DNDEBUG -U_DEBUG"]
	[OBJCFLAGS="$OBJCFLAGS -DNDEBUG -U_DEBUG"]
	[CXXFLAGS="$CXXFLAGS -DNDEBUG -U_DEBUG"]
	AC_SUBST(BUILD_MODE, "release")
fi

AC_ARG_ENABLE([objc], [AC_HELP_STRING([--enable-objc],
	[build the library for Objective-C if an Objective-C compiler is available (default. yes)])],
	enable_objc_is=$enableval,enable_objc_is=yes) 
[test "${HAVE_OBJC}" = "yes" || enable_objc_is="no"]
AM_CONDITIONAL(ENABLE_OBJC, test "${enable_objc_is}" = "yes" )

AC_ARG_ENABLE([cxx], [AC_HELP_STRING([--enable-cxx],
	[build the library for C++ if a C++ compiler is available (default. yes)])],
	enable_cxx_is=$enableval,enable_cxx_is=yes) 
[test "${HAVE_CXX}" = "yes" || enable_cxx_is="no"]
AM_CONDITIONAL(ENABLE_CXX, test "${enable_cxx_is}" = "yes" )

# configure makefiles
AC_ARG_ENABLE([reentrant], [AC_HELP_STRING([--enable-reentrant],
	[define _REENTRANT (default. yes)])],
	enable_reentrant_is=$enableval,enable_reentrant_is=yes)
if test "$enable_reentrant_is" = "yes"
then
	[CFLAGS="$CFLAGS -D_REENTRANT -D_THREAD_SAFE"]
	[OBJCFLAGS="$OBJCFLAGS -D_REENTRANT -D_THREAD_SAFE"]
	[CXXFLAGS="$CXXFLAGS -D_REENTRANT -D_THREAD_SAFE"]
fi

AC_CONFIG_FILES([
	Makefile 
	include/Makefile
	include/qse/Makefile
	include/qse/cmn/Makefile
	include/qse/awk/Makefile
	include/qse/lsp/Makefile
	include/qse/tgp/Makefile
	include/qse/utl/Makefile
	lib/Makefile 
	lib/cmn/Makefile 
	lib/awk/Makefile 
	lib/lsp/Makefile 
	lib/tgp/Makefile 
	lib/utl/Makefile 
	cmd/Makefile 
	cmd/awk/Makefile 
	cmd/lsp/Makefile
	cmd/tgp/Makefile
	test/Makefile 
	test/cmn/Makefile 
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
echo "  Objective-C compiler: ${OBJC} ${OBJCFLAGS}"
echo "  C++ compiler: ${CXX} ${CXXFLAGS}"
echo "  C++ support: ${enable_cxx_is}"
echo "  Wide character: ${enable_wchar_is}" 
echo "  Math library: ${LIBM}"
echo "-------------------------------------------------------------------------"
]
