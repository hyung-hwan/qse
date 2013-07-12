/*
 * $Id: types.h 560 2011-09-06 14:18:36Z hyunghwan.chung $
 *
    Copyright 2006-2012 Chung, Hyung-Hwan.
    This file is part of QSE.

    QSE is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as 
    published by the Free Software Foundation, either version 3 of 
    the License, or (at your option) any later version.

    QSE is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public 
    License along with QSE. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _QSE_TYPES_H_
#define _QSE_TYPES_H_

/**@file
 * This file defines various common basic types designed to be
 * cross-platform. These types are preferred over native data types.
 */

/* WARNING: NEVER CHANGE/DELETE THE FOLLOWING QSE_HAVE_CONFIG_H DEFINITION. 
 *          IT IS USED FOR DEPLOYMENT BY MAKEFILE.AM */
/*#define QSE_HAVE_CONFIG_H*/

#if defined(QSE_HAVE_CONFIG_H)
#	include <qse/config.h>
#elif defined(_WIN32)
#	include <qse/conf-msw.h>
#elif defined(__OS2__)
#	include <qse/conf-os2.h>
#elif defined(__DOS__)
#	include <qse/conf-dos.h>
#elif defined(vms) || defined(__vms)
#	include <qse/conf-vms.h>
#elif defined(macintosh)
#	include <:qse:conf-mac.h> /* class mac os */
#else
#	error Unsupported operating system
#endif

/**
 * The qse_bool_t type defines a boolean type that can represent #QSE_TRUE 
 * and #QSE_FALSE.
 */
enum qse_bool_t
{
	QSE_TRUE  = (0 == 0),
	QSE_FALSE = (0 != 0)
};
typedef enum qse_bool_t qse_bool_t;

/** @typedef qse_int_t
 * The qse_int_t type defines a signed integer type as large as a pointer.
 */
/** @typedef qse_uint_t
 * The qse_uint_t type defines an unsigned integer type as large as a pointer.
 */
#if (defined(hpux) || defined(__hpux) || defined(__hpux__) || \
     (defined(__APPLE__) && defined(__MACH__))) && \
    (QSE_SIZEOF_VOID_P == QSE_SIZEOF_LONG)
	typedef long qse_int_t;
	typedef unsigned long qse_uint_t;
	#define QSE_SIZEOF_INT_T QSE_SIZEOF_LONG
	#define QSE_SIZEOF_UINT_T QSE_SIZEOF_LONG
#elif defined(__SPU__) && (QSE_SIZEOF_VOID_P == QSE_SIZEOF_LONG)
	typedef long qse_int_t;
	typedef unsigned long qse_uint_t;
	#define QSE_SIZEOF_INT_T QSE_SIZEOF_LONG
	#define QSE_SIZEOF_UINT_T QSE_SIZEOF_LONG
#elif QSE_SIZEOF_VOID_P == QSE_SIZEOF_INT
	typedef int qse_int_t;
	typedef unsigned int qse_uint_t;
	#define QSE_SIZEOF_INT_T QSE_SIZEOF_INT
	#define QSE_SIZEOF_UINT_T QSE_SIZEOF_INT
#elif QSE_SIZEOF_VOID_P == QSE_SIZEOF_LONG
	typedef long qse_int_t;
	typedef unsigned long qse_uint_t;
	#define QSE_SIZEOF_INT_T QSE_SIZEOF_LONG
	#define QSE_SIZEOF_UINT_T QSE_SIZEOF_LONG
#elif QSE_SIZEOF_VOID_P == QSE_SIZEOF_LONG_LONG
	typedef long long qse_int_t;
	typedef unsigned long long qse_uint_t;
	#define QSE_SIZEOF_INT_T QSE_SIZEOF_LONG_LONG
	#define QSE_SIZEOF_UINT_T QSE_SIZEOF_LONG_LONG
#elif QSE_SIZEOF_VOID_P == QSE_SIZEOF___INT32
	typedef __int32 qse_int_t;
	typedef unsigned __int32 qse_uint_t;
	#define QSE_SIZEOF_INT_T QSE_SIZEOF___INT32
	#define QSE_SIZEOF_UINT_T QSE_SIZEOF___INT32
#elif QSE_SIZEOF_VOID_P == QSE_SIZEOF___INT32_T
	typedef __int32_t qse_int_t;
	typedef unsigned __int32_t qse_uint_t;
	#define QSE_SIZEOF_INT_T QSE_SIZEOF___INT32_T
	#define QSE_SIZEOF_UINT_T QSE_SIZEOF___INT32_T
#elif QSE_SIZEOF_VOID_P == QSE_SIZEOF___INT64
	typedef __int64 qse_int_t;
	typedef unsigned __int64 qse_uint_t;
	#define QSE_SIZEOF_INT_T QSE_SIZEOF___INT64
	#define QSE_SIZEOF_UINT_T QSE_SIZEOF___INT64
#elif QSE_SIZEOF_VOID_P == QSE_SIZEOF___INT64_T
	typedef __int64_t qse_int_t;
	typedef unsigned __int64_t qse_uint_t;
	#define QSE_SIZEOF_INT_T QSE_SIZEOF___INT64_T
	#define QSE_SIZEOF_UINT_T QSE_SIZEOF___INT64_T
#elif QSE_SIZEOF_VOID_P == QSE_SIZEOF___INT128
	typedef __int128 qse_int_t;
	typedef unsigned __int128 qse_uint_t;
	#define QSE_SIZEOF_INT_T QSE_SIZEOF___INT128
	#define QSE_SIZEOF_UINT_T QSE_SIZEOF___INT128
#elif QSE_SIZEOF_VOID_P == QSE_SIZEOF___INT128_T
	typedef __int128_t qse_int_t;
	typedef unsigned __int128_t qse_uint_t;
	#define QSE_SIZEOF_INT_T QSE_SIZEOF___INT128_T
	#define QSE_SIZEOF_UINT_T QSE_SIZEOF___INT128_T
#else
#	error unsupported pointer size
#endif

/** @typedef qse_long_t
 * The qse_long_t type defines the largest signed integer type that supported. 
 */
/** @typedef qse_ulong_t
 * The qse_ulong_t type defines the largest unsigned integer type supported.
 */
#if QSE_SIZEOF_LONG >= QSE_SIZEOF_LONG_LONG
	typedef long qse_long_t;
	typedef unsigned long qse_ulong_t;
	#define QSE_SIZEOF_LONG_T QSE_SIZEOF_LONG
	#define QSE_SIZEOF_ULONG_T QSE_SIZEOF_LONG
#elif QSE_SIZEOF_LONG_LONG > 0
	typedef long long qse_long_t;
	typedef unsigned long long qse_ulong_t;
	#define QSE_SIZEOF_LONG_T QSE_SIZEOF_LONG_LONG
	#define QSE_SIZEOF_ULONG_T QSE_SIZEOF_LONG_LONG
#elif QSE_SIZEOF___INT64 > 0
	typedef __int64 qse_long_t;
	typedef unsigned __int64 qse_ulong_t;
	#define QSE_SIZEOF_LONG_T QSE_SIZEOF___INT64
	#define QSE_SIZEOF_ULONG_T QSE_SIZEOF___INT64
#elif QSE_SIZEOF___INT64_T > 0
	typedef __int64_t qse_long_t;
	typedef unsigned __int64_t qse_ulong_t;
	#define QSE_SIZEOF_LONG_T QSE_SIZEOF___INT64_T
	#define QSE_SIZEOF_ULONG_T QSE_SIZEOF___INT64_T
#else
	typedef long qse_long_t;
	typedef unsigned long qse_ulong_t;
	#define QSE_SIZEOF_LONG_T QSE_SIZEOF_LONG
	#define QSE_SIZEOF_ULONG_T QSE_SIZEOF_LONG
#endif

/* these two items are revised whenever the size of a 
 * fixed-size integer is determined */
#define QSE_SIZEOF_INTMAX_T  0
#define QSE_SIZEOF_UINTMAX_T 0

/** @typedef qse_int8_t
 * The qse_int8_t defines an 8-bit signed integer type.
 */
/** @typedef qse_uint8_t
 * The qse_uint8_t type defines an 8-bit unsigned integer type.
 */
#if QSE_SIZEOF_CHAR == 1
#	define QSE_HAVE_INT8_T
#	define QSE_HAVE_UINT8_T
	typedef char qse_int8_t;
	typedef unsigned char qse_uint8_t;
#elif QSE_SIZEOF___INT8 == 1
#	define QSE_HAVE_INT8_T
#	define QSE_HAVE_UINT8_T
	typedef __int8 qse_int8_t;
	typedef unsigned __int8 qse_uint8_t;
#elif QSE_SIZEOF___INT8_T == 1
#	define QSE_HAVE_INT8_T
#	define QSE_HAVE_UINT8_T
	typedef __int8_t qse_int8_t;
	typedef __uint8_t qse_uint8_t;
#endif

#ifdef QSE_HAVE_INT8_T
#	define QSE_SIZEOF_INT8_T 1
#	define QSE_SIZEOF_UINT8_T 1
#	undef  QSE_SIZEOF_INTMAX_T
#	undef  QSE_SIZEOF_UINTMAX_T
#	define QSE_SIZEOF_INTMAX_T 1
#	define QSE_SIZEOF_UINTMAX_T 1
#else
#	define QSE_SIZEOF_INT8_T 0
#	define QSE_SIZEOF_UINT8_T 0
#endif

/** @typedef qse_int16_t
 * The qse_int16_t defines an 16-bit signed integer type.
 */
/** @typedef qse_uint16_t
 * The qse_uint16_t type defines an 16-bit unsigned integer type.
 */
#if QSE_SIZEOF_SHORT == 2
#	define QSE_HAVE_INT16_T
#	define QSE_HAVE_UINT16_T
	typedef short qse_int16_t;
	typedef unsigned short qse_uint16_t;
#elif QSE_SIZEOF___INT16 == 2
#	define QSE_HAVE_INT16_T
#	define QSE_HAVE_UINT16_T
	typedef __int16 qse_int16_t;
	typedef unsigned __int16 qse_uint16_t;
#elif QSE_SIZEOF___INT16_T == 2
#	define QSE_HAVE_INT16_T
#	define QSE_HAVE_UINT16_T
	typedef __int16_t qse_int16_t;
	typedef __uint16_t qse_uint16_t;
#endif

#ifdef QSE_HAVE_INT16_T
#	define QSE_SIZEOF_INT16_T 2
#	define QSE_SIZEOF_UINT16_T 2
#	undef  QSE_SIZEOF_INTMAX_T
#	undef  QSE_SIZEOF_UINTMAX_T
#	define QSE_SIZEOF_INTMAX_T 2
#	define QSE_SIZEOF_UINTMAX_T 2
#else
#	define QSE_SIZEOF_INT16_T 0
#	define QSE_SIZEOF_UINT16_T 0
#endif

/** @typedef qse_int32_t
 * The qse_int32_t defines an 32-bit signed integer type.
 */
/** @typedef qse_uint32_t
 * The qse_uint32_t type defines an 32-bit unsigned integer type.
 */
#if QSE_SIZEOF_INT == 4
#	define QSE_HAVE_INT32_T
#	define QSE_HAVE_UINT32_T
	typedef int qse_int32_t;
	typedef unsigned int qse_uint32_t;
#elif QSE_SIZEOF_LONG == 4
#	define QSE_HAVE_INT32_T
#	define QSE_HAVE_UINT32_T
	typedef long qse_int32_t;
	typedef unsigned long qse_uint32_t;
#elif QSE_SIZEOF___INT32 == 4
#	define QSE_HAVE_INT32_T
#	define QSE_HAVE_UINT32_T
	typedef __int32 qse_int32_t;
	typedef unsigned __int32 qse_uint32_t;
#elif QSE_SIZEOF___INT32_T == 4
#	define QSE_HAVE_INT32_T
#	define QSE_HAVE_UINT32_T
	typedef __int32_t qse_int32_t;
	typedef __uint32_t qse_uint32_t;
#endif

#ifdef QSE_HAVE_INT32_T
#	define QSE_SIZEOF_INT32_T 4
#	define QSE_SIZEOF_UINT32_T 4
#	undef  QSE_SIZEOF_INTMAX_T
#	undef  QSE_SIZEOF_UINTMAX_T
#	define QSE_SIZEOF_INTMAX_T 4
#	define QSE_SIZEOF_UINTMAX_T 4
#else
#	define QSE_SIZEOF_INT32_T 0
#	define QSE_SIZEOF_UINT32_T 0
#endif

/** @typedef qse_int64_t
 * The qse_int64_t defines an 64-bit signed integer type.
 */
/** @typedef qse_uint64_t
 * The qse_uint64_t type defines an 64-bit unsigned integer type.
 */
#if QSE_SIZEOF_INT == 8
#	define QSE_HAVE_INT64_T
#	define QSE_HAVE_UINT64_T
	typedef int qse_int64_t;
	typedef unsigned int qse_uint64_t;
#elif QSE_SIZEOF_LONG == 8
#	define QSE_HAVE_INT64_T
#	define QSE_HAVE_UINT64_T
	typedef long qse_int64_t;
	typedef unsigned long qse_uint64_t;
#elif QSE_SIZEOF_LONG_LONG == 8
#	define QSE_HAVE_INT64_T
#	define QSE_HAVE_UINT64_T
	typedef long long qse_int64_t;
	typedef unsigned long long qse_uint64_t;
#elif QSE_SIZEOF___INT64 == 8
#	define QSE_HAVE_INT64_T
#	define QSE_HAVE_UINT64_T
	typedef __int64 qse_int64_t;
	typedef unsigned __int64 qse_uint64_t;
#elif QSE_SIZEOF___INT64_T == 8
#	define QSE_HAVE_INT64_T
#	define QSE_HAVE_UINT64_T
	typedef __int64_t qse_int64_t;
	typedef __uint64_t qse_uint64_t;
#endif

#ifdef QSE_HAVE_INT64_T
#	define QSE_SIZEOF_INT64_T 8
#	define QSE_SIZEOF_UINT64_T 8
#	undef  QSE_SIZEOF_INTMAX_T
#	undef  QSE_SIZEOF_UINTMAX_T
#	define QSE_SIZEOF_INTMAX_T 8
#	define QSE_SIZEOF_UINTMAX_T 8
#else
#	define QSE_SIZEOF_INT64_T 0
#	define QSE_SIZEOF_UINT64_T 0
#endif

#if QSE_SIZEOF_INT == 16
#	define QSE_HAVE_INT128_T
#	define QSE_HAVE_UINT128_T
	typedef int qse_int128_t;
	typedef unsigned int qse_uint128_t;
#elif QSE_SIZEOF_LONG == 16
#	define QSE_HAVE_INT128_T
#	define QSE_HAVE_UINT128_T
	typedef long qse_int128_t;
	typedef unsigned long qse_uint128_t;
#elif QSE_SIZEOF_LONG_LONG == 16
#	define QSE_HAVE_INT128_T
#	define QSE_HAVE_UINT128_T
	typedef long long qse_int128_t;
	typedef unsigned long long qse_uint128_t;
#elif QSE_SIZEOF___INT128 == 16
#	define QSE_HAVE_INT128_T
#	define QSE_HAVE_UINT128_T
	typedef __int128 qse_int128_t;
	typedef unsigned __int128 qse_uint128_t;
#elif (QSE_SIZEOF___INT128_T == 16)
#	define QSE_HAVE_INT128_T
#	define QSE_HAVE_UINT128_T
	typedef __int128_t qse_int128_t;
	typedef __uint128_t qse_uint128_t;
#endif

#ifdef QSE_HAVE_INT128_T
#	define QSE_SIZEOF_INT128_T 16
#	define QSE_SIZEOF_UINT128_T 16
#	undef  QSE_SIZEOF_INTMAX_T
#	undef  QSE_SIZEOF_UINTMAX_T
#	define QSE_SIZEOF_INTMAX_T 16
#	define QSE_SIZEOF_UINTMAX_T 16
#else
#	define QSE_SIZEOF_INT128_T 0
#	define QSE_SIZEOF_UINT128_T 0
#endif

/**
 * The qse_byte_t defines a byte type.
 */
typedef qse_uint8_t qse_byte_t;
#define QSE_SIZEOF_BYTE_T QSE_SIZEOF_UINT8_T

/**
 * The qse_size_t type defines an unsigned integer type that is as large as
 * to hold a pointer value.
 */
#if defined(__SIZE_TYPE__) && defined(__SIZEOF_SIZE_T__)
	typedef __SIZE_TYPE__ qse_size_t;
#	define QSE_SIZEOF_SIZE_T __SIZEOF_SIZE_T__
#else
	typedef qse_uint_t  qse_size_t;
#	define QSE_SIZEOF_SIZE_T QSE_SIZEOF_UINT_T
#endif

/**
 * The qse_ssize_t type defines a signed integer type that is as large as
 * to hold a pointer value.
 */
typedef qse_int_t qse_ssize_t;
#define QSE_SIZEOF_SSIZE_T QSE_SIZEOF_INT_T

/** 
 * The qse_word_t type redefines qse_uint_t. 
 */
typedef qse_uint_t qse_word_t;
#define QSE_SIZEOF_WORD_T QSE_SIZEOF_UINT_T

/**
 * The qse_uintptr_t redefines qse_uint_t to indicate that you are dealing
 * with a pointer.
 */
typedef qse_uint_t qse_uintptr_t;
#define QSE_SIZEOF_UINTPTR_T QSE_SIZEOF_UINT_T

/**
 * The qse_untptr_t redefines qse_int_t to indicate that you are dealing
 * with a pointer.
 */
typedef qse_int_t qse_intptr_t;
#define QSE_SIZEOF_INTPTR_T QSE_SIZEOF_INT_T

/** @typedef qse_intmax_t
 * The qse_llong_t type defines the largest signed integer type supported.
 */
/** @typedef qse_uintmax_t
 * The qse_ullong_t type defines the largest unsigned integer type supported.
 */
#if (QSE_SIZEOF_LONG >= QSE_SIZEOF_LONG_LONG) && \
    (QSE_SIZEOF_LONG >= QSE_SIZEOF_INTMAX_T)
	typedef long qse_intmax_t;
	typedef unsigned long qse_uintmax_t;
	#undef QSE_SIZEOF_INTMAX_T
	#undef QSE_SIZEOF_UINTMAX_T
	#define QSE_SIZEOF_INTMAX_T QSE_SIZEOF_LONG
	#define QSE_SIZEOF_UINTMAX_T QSE_SIZEOF_LONG
#elif (QSE_SIZEOF_LONG_LONG >= QSE_SIZEOF_INTMAX_T) 
	typedef long long qse_intmax_t;
	typedef unsigned long long qse_uintmax_t;
	#undef QSE_SIZEOF_INTMAX_T
	#undef QSE_SIZEOF_UINTMAX_T
	#define QSE_SIZEOF_INTMAX_T QSE_SIZEOF_LONG_LONG
	#define QSE_SIZEOF_UINTMAX_T QSE_SIZEOF_LONG_LONG
#elif (QSE_SIZEOF_INTMAX_T == QSE_SIZEOF_INT128_T)
	typedef qse_int128_t qse_intmax_t;
	typedef qse_uint128_t qse_uintmax_t;
	/* QSE_SIZEOF_INTMAX_T and QSE_SIZEOF_UINTMAX_T are
	 * defined when qse_int128_t is defined */
#elif (QSE_SIZEOF_INTMAX_T == QSE_SIZEOF_INT64_T)
	typedef qse_int64_t qse_intmax_t;
	typedef qse_uint64_t qse_uintmax_t;
	/* QSE_SIZEOF_INTMAX_T and QSE_SIZEOF_UINTMAX_T are
	 * defined when qse_int64_t is defined */
#elif (QSE_SIZEOF_INTMAX_T == QSE_SIZEOF_INT32_T)
	typedef qse_int32_t qse_intmax_t;
	typedef qse_uint32_t qse_uintmax_t;
	/* QSE_SIZEOF_INTMAX_T and QSE_SIZEOF_UINTMAX_T are
	 * defined when qse_int32_t is defined */
#elif (QSE_SIZEOF_INTMAX_T == QSE_SIZEOF_INT16_T)
	typedef qse_int16_t qse_intmax_t;
	typedef qse_uint16_t qse_uintmax_t;
	/* QSE_SIZEOF_INTMAX_T and QSE_SIZEOF_UINTMAX_T are
	 * defined when qse_int16_t is defined */
#elif (QSE_SIZEOF_INTMAX_T == QSE_SIZEOF_INT8_T)
	typedef qse_int8_t qse_intmax_t;
	typedef qse_uint8_t qse_uintmax_t;
	/* QSE_SIZEOF_INTMAX_T and QSE_SIZEOF_UINTMAX_T are
	 * defined when qse_int8_t is defined */
#else
#	error FATAL. THIS MUST NOT HAPPEN
#endif


/** @typedef qse_flt_t
 * The qse_flt_t type defines the largest floating-pointer number type
 * supported.
 */
#if defined(__FreeBSD__)
	/* TODO: check if the support for long double is complete.
	 *       if so, use long double for qse_flt_t */
	typedef double qse_flt_t;
#	define QSE_SIZEOF_FLT_T QSE_SIZEOF_DOUBLE
#elif QSE_SIZEOF_LONG_DOUBLE > QSE_SIZEOF_DOUBLE
	typedef long double qse_flt_t;
#	define QSE_SIZEOF_FLT_T QSE_SIZEOF_LONG_DOUBLE
#else
	typedef double qse_flt_t;
#	define QSE_SIZEOF_FLT_T QSE_SIZEOF_DOUBLE
#endif

/* TODO: qse_fltmax_t to include the quadruple precision floating-point type. 
 *
#if QSE_SIZEOF___FLOAT128 > 0
	typedef __float128 qse_fltmax_t;
#	define QSE_SIZEOF_FLTMAX_T QSE_SIZEOF___FLOAT128
#else
	typedef qse_flt_t qse_fltmax_t;
#	define QSE_SIZEOF_FLTMAX_T QSE_SIZEOF_FLT_T
#endif
 */

/** 
 * The qse_mchar_t type defines a multi-byte character type.
 */
typedef char qse_mchar_t;
#define QSE_SIZEOF_MCHAR_T QSE_SIZEOF_CHAR 

/**
 * The qse_mcint_t defines a type that can hold a qse_mchar_t value and 
 * #QSE_MCHAR_EOF.
 */
typedef int qse_mcint_t;
#define QSE_SIZEOF_MCINT_T QSE_SIZEOF_INT 

/** @typedef qse_wchar_t
 * The qse_wchar_t type defines a wide character type. 
 */
/** @typedef qse_wcint_t
 * The qse_wcint_t type defines a type that can hold a qse_wchar_t value and 
 * #QSE_WCHAR_EOF.
 */
#if defined(__cplusplus) && (defined(__WATCOMC__) && (__WATCOMC__ < 1200))
	/* WATCOM C++ before OpenWatcom */

	typedef long char qse_wchar_t;
	typedef long char qse_wcint_t;

#elif defined(__cplusplus) && !( \
      (defined(_MSC_VER) && !defined(_NATIVE_WCHAR_T_DEFINED)) || \
      (defined(__WATCOMC__) && (__WATCOMC__ < 1200)) || \
      defined(_SCO_DS) \
    )
	/* C++ */

	typedef wchar_t qse_wchar_t;
	typedef wchar_t qse_wcint_t;

	/* all the way down from here for C */
#elif defined(__GNUC__) && defined(__WCHAR_TYPE__) && defined(__WINT_TYPE__)
	typedef __WCHAR_TYPE__ qse_wchar_t;
	typedef __WINT_TYPE__ qse_wcint_t;
#elif (QSE_SIZEOF_WCHAR_T == 2) || (QSE_SIZEOF_WCHAR_T == 0)
	typedef unsigned short qse_wchar_t;
	typedef unsigned short qse_wcint_t;
#elif (QSE_SIZEOF_WCHAR_T == 4)
#	if defined(vms) || defined(__vms)
		typedef unsigned int qse_wchar_t;
		typedef int qse_wcint_t;
#	elif defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)
		typedef int qse_wchar_t;
		typedef int qse_wcint_t;
#	elif (defined(sun) || defined(__sun) || defined(__linux))
#		if defined(_LP64)
			typedef int qse_wchar_t;
			typedef int qse_wcint_t;
#		else
			typedef long qse_wchar_t;
			typedef long qse_wcint_t;
#		endif
#	elif defined(__APPLE__) && defined(__MACH__)
		typedef int qse_wchar_t;
		typedef int qse_wcint_t;
#	elif defined(hpux) || defined(__hpux) || defined(__hpux__)
#		if defined(__HP_cc) || defined(__HP_aCC)
			typedef unsigned int qse_wchar_t;
#		else
			typedef int qse_wchar_t;
#		endif
		typedef int qse_wcint_t;
#	elif QSE_SIZEOF_LONG == 4
		typedef long qse_wchar_t;
		typedef long qse_wcint_t;
#	elif QSE_SIZEOF_INT == 4
		typedef int qse_wchar_t;
		typedef int qse_wcint_t;
#	else
#		error No supported data type for wchar_t
#	endif
#else
#	error Unsupported size of wchar_t
#endif

/** @typedef qse_char_t
 * The qse_char_t type defines a character type.
 */
/** @typedef qse_cint_t
 * The qse_cint_t typep defines a type that can hold a qse_char_t value and 
 * #QSE_CHAR_EOF.
 */
#if defined(QSE_CHAR_IS_MCHAR)
	typedef qse_mchar_t qse_char_t;
	typedef qse_mcint_t qse_cint_t;
#elif defined(QSE_CHAR_IS_WCHAR)
	typedef qse_wchar_t qse_char_t;
	typedef qse_wcint_t qse_cint_t;
#else
	/* If the character type is not determined in the conf_xxx files */

#	if defined(_WIN32)
#		if defined(UNICODE) || defined(_UNICODE)
#			define QSE_CHAR_IS_WCHAR
			typedef qse_wchar_t qse_char_t;
			typedef qse_wcint_t qse_cint_t;
#		else
#			define QSE_CHAR_IS_MCHAR
			typedef qse_mchar_t qse_char_t;
			typedef qse_mcint_t qse_cint_t;
#		endif
#	else
#		error Cannot determine the character type to use
#	endif
#endif

#if defined(QSE_CHAR_IS_WCHAR) && defined(_WIN32) 
	/* Special definiton to use Unicode APIs on Windows */
#	ifndef UNICODE
#		define UNICODE
#	endif
#	ifndef _UNICODE
#		define _UNICODE
#	endif
#endif

typedef struct qse_link_t qse_link_t;
struct qse_link_t
{
	qse_link_t* link;
};

/**
 * The qse_mxstr_t type defines a structure combining a pointer to a character
 * string and the number of characters. It is designed to be interchangeable
 * with the #qse_mcstr_t type except the constness on the @a ptr field.
 */
struct qse_mxstr_t
{
	qse_mchar_t* ptr; /**< pointer to a character string */
	qse_size_t   len; /**< the number of characters */
};
typedef struct qse_mxstr_t qse_mxstr_t;

/**
 * The qse_wxstr_t type defines a structure combining a pointer to a character
 * string and the number of characters. It is designed to be interchangeable
 * with the #qse_wcstr_t type except the constness on the @a ptr field.
 */
struct qse_wxstr_t
{
	qse_wchar_t* ptr; /**< pointer to a character string */
	qse_size_t   len; /**< the number of characters */
};
typedef struct qse_wxstr_t qse_wxstr_t;

/**
 * The qse_mcstr_t type defines a structure combining a pointer to
 * a constant character string and the number of characters.
 * It is designed to be interchangeable with the #qse_mxstr_t type
 * except the constness on the @a ptr field.
 */
struct qse_mcstr_t
{
	const qse_mchar_t* ptr; /**< pointer to a const character string */
	qse_size_t         len; /**< the number of characters */
};
typedef struct qse_mcstr_t qse_mcstr_t;

/**
 * The qse_wcstr_t type defines a structure combining a pointer to
 * a constant character string and the number of characters.
 * It is designed to be interchangeable with the #qse_wxstr_t type
 * except the constness on the @a ptr field.
 */
struct qse_wcstr_t
{
	const qse_wchar_t* ptr; /**< pointer to a const character string */
	qse_size_t         len; /**< the number of characters */
};
typedef struct qse_wcstr_t qse_wcstr_t;

#if defined(QSE_CHAR_IS_MCHAR)
	typedef qse_mxstr_t qse_xstr_t;
	typedef qse_mcstr_t qse_cstr_t;
#else
	typedef qse_wxstr_t qse_xstr_t;
	typedef qse_wcstr_t qse_cstr_t;
#endif

/** 
 * The qse_cptl_t type defines a pair type of a constant pointer and a length.
 */
struct qse_cptl_t
{
	const void* ptr; /**< pointer */
	qse_size_t  len; /**< length */
};
typedef struct qse_cptl_t qse_cptl_t;

/** 
 * The qse_xptl_t type defines a pair type of a pointer and a length.
 */
struct qse_xptl_t
{
	void*       ptr; /**< pointer */
	qse_size_t  len; /**< length */
};
typedef struct qse_xptl_t qse_xptl_t;

/** 
 * allocate a memory chunk of the size @a n.
 * @return pointer to a memory chunk on success, QSE_NULL on failure.
 */
typedef void* (*qse_mmgr_alloc_t)   (void* ctx, qse_size_t n);
/** 
 * resize a memory chunk pointed to by @a ptr to the size @a n.
 * @return pointer to a memory chunk on success, QSE_NULL on failure.
 */
typedef void* (*qse_mmgr_realloc_t) (void* ctx, void* ptr, qse_size_t n);
/**
 * free a memory chunk pointed to by @a ptr.
 */
typedef void  (*qse_mmgr_free_t)    (void* ctx, void* ptr);

/**
 * The qse_mmgr_t type defines the memory management interface.
 * As the type is merely a structure, it is just used as a single container
 * for memory management functions with a pointer to user-defined data. 
 * The user-defined data pointer @a ctx is passed to each memory management 
 * function whenever it is called. You can allocate, reallocate, and free 
 * a memory chunk.
 *
 * For example, a qse_xxx_open() function accepts a pointer of the qse_mmgr_t
 * type and the xxx object uses it to manage dynamic data within the object. 
 */
struct qse_mmgr_t
{
	qse_mmgr_alloc_t   alloc;   /**< allocation function */
	qse_mmgr_realloc_t realloc; /**< resizing function */
	qse_mmgr_free_t    free;    /**< disposal function */
	void*              ctx;     /**< user-defined data pointer */
};
typedef struct qse_mmgr_t qse_mmgr_t;

typedef qse_size_t (*qse_cmgr_mbtowc_t) (
	const qse_mchar_t* mb, 
	qse_size_t         size,
	qse_wchar_t*       wc
);

typedef qse_size_t (*qse_cmgr_wctomb_t) (
	qse_wchar_t  wc,
	qse_mchar_t* mb,
	qse_size_t   size
);

/**
 * The qse_cmgr_t type defines the character-level interface to 
 * multibyte/wide-character conversion. This interface doesn't 
 * provide any facility to store conversion state in a context
 * independent manner. This leads to the limitation that it can
 * handle a stateless multibyte encoding only.
 */
struct qse_cmgr_t
{
	qse_cmgr_mbtowc_t mbtowc;
	qse_cmgr_wctomb_t wctomb;
};

typedef struct qse_cmgr_t qse_cmgr_t;

#if 0
struct qse_tmgr_t
{
	int gettimeoffset
};
typedef struct qse_tmgr_t qse_tmgr_t;
#endif

/**
 * The #qse_foff_t type defines an integer that can represent a file offset.
 * Depending on your system, it's defined to one of #qse_int64_t, #qse_int32_t,
 * and #qse_int16_t.
 */
#if defined(QSE_HAVE_INT64_T) && (QSE_SIZEOF_OFF64_T==8)
	typedef qse_int64_t qse_foff_t;
#	define QSE_SIZEOF_FOFF_T QSE_SIZEOF_INT64_T
#elif defined(QSE_HAVE_INT64_T) && (QSE_SIZEOF_OFF_T==8)
	typedef qse_int64_t qse_foff_t;
#	define QSE_SIZEOF_FOFF_T QSE_SIZEOF_INT64_T
#elif defined(QSE_HAVE_INT32_T) && (QSE_SIZEOF_OFF_T==4)
	typedef qse_int32_t qse_foff_t;
#	define QSE_SIZEOF_FOFF_T QSE_SIZEOF_INT32_T
#elif defined(QSE_HAVE_INT16_T) && (QSE_SIZEOF_OFF_T==2)
	typedef qse_int16_t qse_foff_t;
#	define QSE_SIZEOF_FOFF_T QSE_SIZEOF_INT16_T
#else
	typedef qse_int32_t qse_foff_t; /* this line is for doxygen */
#    error Unsupported platform
#endif

/** 
 * The qse_ubi_t type defines a union type that includes most of built-in 
 * data types and numeric types defined in the library.
 */
union qse_ubi_t
{
	char           c;
	unsigned char  uc;
	short          s;
	unsigned short us;
	int            i;
	unsigned int   ui;
	long           l;
	unsigned long  ul;
#if defined(QSE_SIZEOF_LONG_LONG) && (QSE_SIZEOF_LONG_LONG > 0)
	long long      ll;
	unsigned long long ull;
#endif
	float          f;
	double         d;
#if defined(QSE_SIZEOF_LONG_DOUBLE) && (QSE_SIZEOF_LONG_DOUBLE > 0)
	long double    ld;
#endif
	void*          ptr;

	qse_byte_t     byte;
	qse_int_t      sint;
	qse_uint_t     uint;
	qse_long_t     slong;
	qse_ulong_t    ulong;
	qse_size_t     size;
	qse_ssize_t    ssize;
	qse_word_t     word;
	qse_intptr_t   intptr;
	qse_uintptr_t  uintptr;
	qse_intmax_t   intmax;
	qse_uintmax_t  uintmax;
	qse_flt_t      flt;

	qse_char_t     ch;
	qse_mchar_t    mch;
	qse_wchar_t    wch;
	qse_cint_t     cint;
	qse_mcint_t    mcint;
	qse_wcint_t    wcint;

	qse_int8_t     int8;
	qse_uint8_t    uint8;
	qse_int16_t    int16;
	qse_uint16_t   uint16;
	qse_int32_t    int32;
	qse_uint32_t   uint32;
#if defined(QSE_HAVE_INT64_T)
	qse_int64_t    int64;
	qse_uint64_t   uint64;
#endif
#if defined(QSE_HAVE_INT128_T)
	qse_int128_t   int128;
	qse_uint128_t  uint128;
#endif
	qse_foff_t     foff;
};
typedef union qse_ubi_t qse_ubi_t;

/*
 * Note QSE_MBLEN_MAX was set to 2 when autoconf ran for _WIN32 
 * with mingw32. This is bad since it can't handle UTF-8. Here are
 * some redefinitions. (_WIN64 with mingw64 gave me 5 though).
 */
#if (QSE_SIZEOF_WCHAR_T == 2) && (QSE_MBLEN_MAX < 3)
#	undef QSE_MBLEN_MAX
#	define QSE_MBLEN_MAX 3
#elif (QSE_SIZEOF_WCHAR_T == 4) && (QSE_MBLEN_MAX < 6)
#	undef QSE_MBLEN_MAX 
#	define QSE_MBLEN_MAX 6
#endif

#endif
