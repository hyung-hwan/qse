#
# $Id: configure.ac,v 1.81 2007-04-04 14:45:52 bacon Exp $
# 

AC_PREREQ(2.53)
AC_INIT([ase], [deb-0.1.0])
AC_REVISION([$Revision: 1.81 $])
AC_CONFIG_HEADER([cmn/conf_unx.h])

# Checks for programs.
AC_PROG_CC
AC_PROG_RANLIB

#AC_PROG_CXX

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
	AC_SUBST(BUILDMODE, "debug")
else
	[CFLAGS="$CFLAGS -DNDEBUG -U_DEBUG"]
	AC_SUBST(BUILDMODE, "release")
fi

# Configure makefiles
AC_ARG_ENABLE([reentrant], [AC_HELP_STRING([--enable-reentrant],
	[define _REENTRANT (default. yes)])],
	enable_reentrant_is=$enableval,enable_reentrant_is=yes)
if test "$enable_reentrant_is" = "yes"
then
	[CFLAGS="$CFLAGS -D_REENTRANT -D_THREAD_SAFE"]
fi

#
# JNI Configuration - http://autoconf-archive.cryp.to/ac_jni_include_dir.html
#

AC_DEFUN([AC_JNI_INCLUDE_DIR],[

JNI_INCLUDE_DIRS=""

#test "x$JAVAC" = x && AC_MSG_ERROR(['\$JAVAC' undefined])
test "x$JAVAC" = x && JAVAC=javac
AC_PATH_PROG(_ACJNI_JAVAC, $JAVAC, no)
test "x$_ACJNI_JAVAC" = xno && AC_MSG_ERROR([$JAVAC could not be found in path])

host_os=`uname -s | tr [A-Z] [a-z]`

_ACJNI_FOLLOW_SYMLINKS("$_ACJNI_JAVAC")
_JTOPDIR=`echo "$_ACJNI_FOLLOWED" | sed -e 's://*:/:g' -e 's:/[[^/]]*$::'`
case "$host_os" in
        darwin*)        _JTOPDIR=`echo "$_JTOPDIR" | sed -e 's:/[[^/]]*$::'`
                        _JINC="$_JTOPDIR/Headers";;
        *)              _JINC="$_JTOPDIR/include";;
esac
if test -f "$_JINC/jni.h"; then
        JNI_INCLUDE_DIRS="$JNI_INCLUDE_DIRS $_JINC"
else
        _JTOPDIR=`echo "$_JTOPDIR" | sed -e 's:/[[^/]]*$::'`
        if test -f "$_JTOPDIR/include/jni.h"; then
                JNI_INCLUDE_DIRS="$JNI_INCLUDE_DIRS $_JTOPDIR/include"
        else
                AC_MSG_ERROR([cannot find java include files])
        fi
fi

# get the likely subdirectories for system specific java includes
case "$host_os" in
bsdi*)          _JNI_INC_SUBDIRS="bsdos";;
linux*)         _JNI_INC_SUBDIRS="linux genunix";;
osf*)           _JNI_INC_SUBDIRS="alpha";;
solaris*)       _JNI_INC_SUBDIRS="solaris";;
mingw*)                 _JNI_INC_SUBDIRS="win32";;
cygwin*)                _JNI_INC_SUBDIRS="win32";;
*)              _JNI_INC_SUBDIRS="genunix";;
esac

# add any subdirectories that are present
for JINCSUBDIR in $_JNI_INC_SUBDIRS
do
        if test -d "$_JTOPDIR/include/$JINCSUBDIR"; then
                JNI_INCLUDE_DIRS="$JNI_INCLUDE_DIRS $_JTOPDIR/include/$JINCSUBDIR"
        fi
done
])

# _ACJNI_FOLLOW_SYMLINKS <path>
# Follows symbolic links on <path>,
# finally setting variable _ACJNI_FOLLOWED
# --------------------
AC_DEFUN([_ACJNI_FOLLOW_SYMLINKS],[
# find the include directory relative to the javac executable
_cur="$1"
while ls -ld "$_cur" 2>/dev/null | grep " -> " >/dev/null; do
        AC_MSG_CHECKING(symlink for $_cur)
        _slink=`ls -ld "$_cur" | sed 's/.* -> //'`
        case "$_slink" in
        /*) _cur="$_slink";;
        # 'X' avoids triggering unwanted echo options.
        *) _cur=`echo "X$_cur" | sed -e 's/^X//' -e 's:[[^/]]*$::'`"$_slink";;
        esac
        AC_MSG_RESULT($_cur)
done
_ACJNI_FOLLOWED="$_cur"
])# _ACJNI

AC_JNI_INCLUDE_DIR

for JNI_INCLUDE_DIR in $JNI_INCLUDE_DIRS
do
	CFLAGS="$CFLAGS -I$JNI_INCLUDE_DIR"
done


AC_CONFIG_FILES([makefile cmn/makefile awk/makefile lsp/makefile utl/makefile test/awk/makefile test/lsp/makefile])
AC_OUTPUT
