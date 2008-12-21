AC_PREREQ(2.59)
AC_INIT([ase],[0.5.0],[bacon@abiyo.net])
AC_CONFIG_HEADER([include/ase/config.h])
AC_CONFIG_AUX_DIR(autoconf)

dnl uses CC and CPP for compilation tests with .c test programs.
AC_LANG(C)

dnl initializes automake
AM_INIT_AUTOMAKE([-Wall -Werror foreign])

dnl determines a C compiler to use
AC_PROG_CC

dnl determines a C++ compiler to use
AC_PROG_CXX

dnl checks if the C++ compiler exists in PATH. 
AC_CHECK_PROG(HAVE_CXX, $CXX, yes, no)

AC_PROG_LIBTOOL
AC_SUBST(LIBTOOL_DEPS)

AC_CHECK_TOOL(RANLIB, ranlib)
AC_CHECK_TOOL(AR, ar)

AC_CHECK_TOOL(GREP, grep)
AC_CHECK_TOOL(RM, rm)
AC_CHECK_TOOL(SED, sed)
AC_CHECK_TOOL(CJ, gcj)

dnl overrides the default CFLAGS setting
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
#
#if test "$ac_test_CJFLAGS" = "set"
#then
#	CJFLAGS=$ac_save_CJFLAGS
#else
#	if test "$CJ" = "gcj"
#	then
#		CJFLAGS="-O2"
#	else
#		CJFLAGS=
#	fi
#fi

dnl make visible the 64bit interface to the file system
CFLAGS="$CFLAGS -D_LARGEFILE64_SOURCE -DASE_HAVE_CONFIG_H"
CXXFLAGS="$CXXFLAGS -D_LARGEFILE64_SOURCE -DASE_HAVE_CONFIG_H"

dnl Checks for the math library (is -lm needed?)
AC_CHECK_LIBM
AC_SUBST(LIBM, $LIBM)

dnl Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([stddef.h wchar.h wctype.h sys/syscall.h])

dnl Checks data types
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

OLDLIBS="$LIBS"
LIBS="$LIBM $LIBS"
AC_CHECK_FUNCS([powl sinl cosl tanl atanl atan2l logl expl sqrtl])
AC_CHECK_FUNCS([pow sin cos tan atan atan2 log exp sqrt])
AC_CHECK_FUNCS([powf sinf cosf tanf atanf atan2f logf expf sqrtf])
LIBS="$OLDLIBS"

dnl Checks the size of primitive data types
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

AC_DEFINE_UNQUOTED(ASE_SIZEOF_CHAR, ${ac_cv_sizeof_char}, [sizeof(char)])
AC_DEFINE_UNQUOTED(ASE_SIZEOF_SHORT, ${ac_cv_sizeof_short}, [sizeof(short)])
AC_DEFINE_UNQUOTED(ASE_SIZEOF_INT, ${ac_cv_sizeof_int}, [sizeof(int)])
AC_DEFINE_UNQUOTED(ASE_SIZEOF_LONG, ${ac_cv_sizeof_long}, [sizeof(long)])
AC_DEFINE_UNQUOTED(ASE_SIZEOF_LONG_LONG, ${ac_cv_sizeof_long_long}, [sizeof(long long)])
AC_DEFINE_UNQUOTED(ASE_SIZEOF___INT8, ${ac_cv_sizeof___int8}, [sizeof(__int8)])
AC_DEFINE_UNQUOTED(ASE_SIZEOF___INT16, ${ac_cv_sizeof___int16}, [sizeof(__int16)])
AC_DEFINE_UNQUOTED(ASE_SIZEOF___INT32, ${ac_cv_sizeof___int32}, [sizeof(__int32)])
AC_DEFINE_UNQUOTED(ASE_SIZEOF___INT64, ${ac_cv_sizeof___int64}, [sizeof(__int64)])
AC_DEFINE_UNQUOTED(ASE_SIZEOF___INT128, ${ac_cv_sizeof___int128}, [sizeof(__int128)])
AC_DEFINE_UNQUOTED(ASE_SIZEOF_VOID_P, ${ac_cv_sizeof_void_p}, [sizeof(void*)])
AC_DEFINE_UNQUOTED(ASE_SIZEOF_FLOAT, ${ac_cv_sizeof_float}, [sizeof(float)])
AC_DEFINE_UNQUOTED(ASE_SIZEOF_DOUBLE, ${ac_cv_sizeof_double}, [sizeof(double)])
AC_DEFINE_UNQUOTED(ASE_SIZEOF_LONG_DOUBLE, ${ac_cv_sizeof_long_double}, [sizeof(long double)])
AC_DEFINE_UNQUOTED(ASE_SIZEOF_WCHAR_T, ${ac_cv_sizeof_wchar_t}, [sizeof(wchar_t)])

AC_DEFINE_UNQUOTED(ASE_VERSION, "${VERSION}", [package version])
AC_DEFINE_UNQUOTED(ASE_VERSION_MAJOR, $(echo ${VERSION} | cut -d. -f1), [major version number])
AC_DEFINE_UNQUOTED(ASE_VERSION_MINOR, $(echo ${VERSION} | cut -d. -f2), [minor version number])
AC_DEFINE_UNQUOTED(ASE_VERSION_PATCH, $(echo ${VERSION} | cut -d. -f3), [patch level])
AC_DEFINE_UNQUOTED(ASE_AUTHOR,"${PACKAGE_BUGREPORT}", [author's email address])

AC_C_BIGENDIAN(
	[AC_DEFINE([ASE_ENDIAN_BIG],[],[Big Endian])],
	[AC_DEFINE([ASE_ENDIAN_LITTLE],[],[Little Endian])],
	[AC_DEFINE([ASE_ENDIAN_UNKNOWN],[],[Unknown Endian])])

dnl define extra options
AC_ARG_ENABLE([wchar], [AC_HELP_STRING([--enable-wchar],
	[use wchar_t a default charater type when enabled (default. yes)])],
	enable_wchar_is=$enableval,enable_wchar_is=yes)
test "${ac_cv_sizeof_wchar_t}" = "0" && enable_wchar_is=no
if test "${enable_wchar_is}" = "yes"
then
	AC_DEFINE([ASE_CHAR_IS_WCHAR],[],[char is wchar])
else
	AC_DEFINE([ASE_CHAR_IS_MCHAR],[],[char is mchar])
fi

AC_ARG_ENABLE([syscall], [AC_HELP_STRING([--enable-syscall],
	[use the syscall() function to call system calls (default. no)])],
	enable_syscall_is=$enableval,enable_syscall_is=no)
if test "${enable_syscall_is}" = "yes"
then
	AC_DEFINE([ASE_USE_SYSCALL],[],[use the syscall() function])
fi

AC_ARG_ENABLE([debug], [AC_HELP_STRING([--enable-debug],
	[build the library in the debug mode (default. no)])],
	enable_debug_is=$enableval,enable_debug_is=no)
if test "$enable_debug_is" = "yes"
then
	[CFLAGS="$CFLAGS -g -D_DEBUG -UNDEBUG"]
	[CXXFLAGS="$CXXFLAGS -g -D_DEBUG -UNDEBUG"]
	[CJFLAGS="$CJFLAGS -g"]
	AC_SUBST(BUILD_MODE, "debug")
else
	[CFLAGS="$CFLAGS -DNDEBUG -U_DEBUG"]
	[CXXFLAGS="$CXXFLAGS -DNDEBUG -U_DEBUG"]
	[CJFLAGS="$CJFLAGS"]
	AC_SUBST(BUILD_MODE, "release")
fi

AC_ARG_ENABLE([cxx], [AC_HELP_STRING([--enable-cxx],
	[build the library for C++ if a C++ compiler is available (default. yes)])],
	enable_cxx_is=$enableval,enable_cxx_is=yes) 
[test "${HAVE_CXX}" = "yes" || enable_cxx_is="no"]
AM_CONDITIONAL(ENABLE_CXX, test "${enable_cxx_is}" = "yes" )

# Configure makefiles
AC_ARG_ENABLE([reentrant], [AC_HELP_STRING([--enable-reentrant],
	[define _REENTRANT (default. yes)])],
	enable_reentrant_is=$enableval,enable_reentrant_is=yes)
if test "$enable_reentrant_is" = "yes"
then
	[CFLAGS="$CFLAGS -D_REENTRANT -D_THREAD_SAFE"]
	[CXXFLAGS="$CXXFLAGS -D_REENTRANT -D_THREAD_SAFE"]
fi

# Java & JNI Configuration 
AC_PATH_PROG(JAVAC_PATH, javac, :)
AC_PATH_PROG(JAR_PATH, jar, :)

if test "$JAVAC_PATH" != ":"
then
	CUR_JAVAC_PATH="$JAVAC_PATH"

	while ls -ld "$CUR_JAVAC_PATH" 2>/dev/null | grep " -> " >/dev/null
	do
		AC_MSG_CHECKING(symlink for $CUR_JAVAC_PATH)
		REAL_JAVAC_PATH=`ls -ld "$CUR_JAVAC_PATH" | sed 's/.* -> //'`

		case "$REAL_JAVAC_PATH" in
		/*) CUR_JAVAC_PATH="$REAL_JAVAC_PATH";;
		*) CUR_JAVAC_PATH=`echo "X$CUR_JAVAC_PATH" | sed -e 's/^X//' -e 's:[[^/]]*$::'`"$REAL_JAVAC_PATH";;
		esac

		AC_MSG_RESULT($CUR_JAVAC_PATH)
	done

	# check if the javavm wrapper is in use 
	case "$REAL_JAVAC_PATH" in
	*/javavm)
		JAVA_DIR="$REAL_JAVAC_PATH"
		while true
		do
			JAVA_DIR=`echo "$JAVA_DIR" | sed -e 's://*:/:g' -e 's:/[[^/]]*$::'`
			if test "$JAVA_DIR" = "" 
			then
				break
			fi

			AC_MSG_CHECKING($JAVA_DIR/etc/javavms)
			if test -f "$JAVA_DIR/etc/javavms"
			then
				# takes the first jvm configured
				CUR_JAVAC_PATH=`cat $JAVA_DIR/etc/javavms | head -1`
				AC_MSG_RESULT($CUR_JAVAC_PATH)
				break;
			else
				AC_MSG_RESULT(no)
			fi
		done
		;;
	esac

	JAVA_DIR="$CUR_JAVAC_PATH"

	while true
	do
		JAVA_DIR=`echo "$JAVA_DIR" | sed -e 's://*:/:g' -e 's:/[[^/]]*$::'`
		if test "$JAVA_DIR" = "" 
		then
			break
		fi

		AC_MSG_CHECKING($JAVA_DIR/include/jni.h)
		if test -f "$JAVA_DIR/include/jni.h"
		then
			AC_MSG_RESULT(yes)
			JNI_INCLUDE_DIRS="$JNI_INCLUDE_DIRS $JAVA_DIR/include"
			break;
		else
			AC_MSG_RESULT(no)
		fi
	done

	for i in $JNI_INCLUDE_DIRS
	do
		JNI_MD_H=`find "$i" -name jni_md.h -print`
		if test "$JNI_MD_H" != "" 
		then
			tmp=`echo "$JNI_MD_H" | sed -e 's://*:/:g' -e 's:/[[^/]]*$::'`
			JNI_MD_INCLUDE_DIRS="$JNI_MD_INCLUDE_DIRS $tmp"
		fi
	done

	JNI_INCLUDE_DIRS="$JNI_INCLUDE_DIRS $JNI_MD_INCLUDE_DIRS"

	for i in $JNI_INCLUDE_DIRS
	do
		CFLAGS_JNI="$CFLAGS_JNI -I$i"	
	done

	BUILD_JNI="jni"
else
	JAVAC_PATH=""
	CFLAGS_JNI=""
	BUILD_JNI=""
fi

if test "$CJ" != ""
then
	BUILD_CJ="cj"
else
	BUILD_CJ=
fi

AC_SUBST(CFLAGS_JNI, $CFLAGS_JNI)
AC_SUBST(JAVAC, $JAVAC_PATH)
AC_SUBST(JAR, $JAR_PATH)
AC_SUBST(BUILD_JNI, $BUILD_JNI)
AC_SUBST(CJFLAGS, $CJFLAGS)
AC_SUBST(BUILD_CJ, $BUILD_CJ)

AC_CONFIG_FILES([
	makefile 
	include/makefile
	include/ase/makefile
	include/ase/cmn/makefile
	include/ase/awk/makefile
	include/ase/lsp/makefile
	include/ase/tgp/makefile
	include/ase/utl/makefile
	lib/makefile 
	lib/cmn/makefile 
	lib/awk/makefile 
	lib/lsp/makefile 
	lib/tgp/makefile 
	lib/utl/makefile 
	cmd/makefile 
	cmd/awk/makefile 
	cmd/lsp/makefile
	cmd/tgp/makefile])
AC_OUTPUT

[
echo 
echo "-[SUMMARY]---------------------------------------------------------------"
echo "Configured for ${host}"
echo "  Build mode : ${BUILD_MODE}"
echo "  Installation directory: ${prefix}"
echo "  C compiler: ${CC} ${CFLAGS}"
echo "  C++ compiler: ${CXX} ${CXXFLAGS}"
echo "  C++ support: ${enable_cxx_is}"
echo "  Wide character: ${enable_wchar_is}" 
echo "  Math library: ${LIBM}"
echo "-------------------------------------------------------------------------"
]
