#
# $Id: configure.ac,v 1.4 2007/05/25 07:30:26 bacon Exp $
# 

AC_PREREQ(2.53)
AC_INIT([ase], [deb-0.1.0])
AC_REVISION([$Revision: 1.4 $])
AC_CONFIG_HEADER([cmn/conf_unx.h])
AC_CONFIG_AUX_DIR(config)

# Checks for programs.
AC_PROG_CC
#AC_PROG_RANLIB
AC_PROG_LIBTOOL

AC_SUBST(LIBTOOL_DEPS)

# Overrides the default CFLAGS setting
if test "$ac_test_CFLAGS" = "set"
then
	CFLAGS=$ac_save_CFLAGS
else
	if test "$GCC" = "yes"
	then
		CFLAGS="-O2"
	else
		CFLAGS=
	fi
fi

if test "$ac_test_CXXFLAGS" = "set"
then
	CXXFLAGS=$ac_save_CXXFLAGS
else
	if test "$GCC" = "yes"
	then
		CXXFLAGS="-O2"
	else
		CXXFLAGS=
	fi
fi

# Checks for header files.
AC_HEADER_STDC

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
AC_DEFINE_UNQUOTED(ASE_SIZEOF_WCHAR_T,${ac_cv_sizeof_wchar_t})

# Defines extra options

AC_ARG_ENABLE([wchar], [AC_HELP_STRING([--enable-wchar],
	[use wchar_t a default charater type when enabled (default. yes)])],
	enable_wchar_is=$enableval,enable_wchar_is=yes)
if test "$enable_wchar_is" = "yes"
then
	AC_DEFINE([ASE_CHAR_IS_WCHAR],[],[char is wchar])
else
	AC_DEFINE([ASE_CHAR_IS_MCHAR],[],[char is mchar])
fi

AC_ARG_ENABLE([mode], [AC_HELP_STRING([--enable-debug],
	[build the library in the debug mode (default. no)])],
	enable_debug_is=$enableval,enable_debug_is=no)
if test "$enable_debug_is" = "yes"
then
	[CFLAGS="$CFLAGS -g -D_DEBUG -UNDEBUG"]
	[CXXFLAGS="$CXXFLAGS -g -D_DEBUG -UNDEBUG"]
	AC_SUBST(BUILDMODE, "debug")
else
	[CFLAGS="$CFLAGS -DNDEBUG -U_DEBUG"]
	[CXXFLAGS="$CXXFLAGS -DNDEBUG -U_DEBUG"]
	AC_SUBST(BUILDMODE, "release")
fi

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
			JAVA_DIR=`echo "$JAVA_DIR" | sed -e 's/\/\/*/\//g' -e 's/\/[^/]*$//'`
			if test "$JAVA_DIR" = "" 
			then
				break
			fi

			AC_MSG_CHECKING($JAVA_DIR/etc/javavms)
			if test -f "$JAVA_DIR/etc/javavms"
			then
				AC_MSG_RESULT(yes)
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
		JAVA_DIR=`echo "$JAVA_DIR" | sed -e 's/\/\/*/\//g' -e 's/\/[^/]*$//'`
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
			tmp=`echo "$JNI_MD_H" | sed -e 's/\/\/*/\//g' -e 's/\/[^/]*$//'`
			JNI_MD_INCLUDE_DIRS="$JNI_MD_INCLUDE_DIRS $tmp"
		fi
	done

	JNI_INCLUDE_DIRS="$JNI_INCLUDE_DIRS $JNI_MD_INCLUDE_DIRS"

	for i in $JNI_INCLUDE_DIRS
	do
		CFLAGS_JNI="$CFLAGS_JNI -I$i"	
	done

	JNI="jni"
else
	JAVAC_PATH=""
	CFLAGS_JNI=""
	JNI=""
fi

AC_SUBST(CFLAGS_JNI, $CFLAGS_JNI)
AC_SUBST(JAVAC, $JAVAC_PATH)
AC_SUBST(JAR, $JAR_PATH)
AC_SUBST(JNI, $JNI)

AC_CONFIG_FILES([makefile cmn/makefile awk/makefile lsp/makefile utl/makefile test/awk/makefile test/lsp/makefile])
AC_OUTPUT
