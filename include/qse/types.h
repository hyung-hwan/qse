/*
 * $Id: types.h 560 2011-09-06 14:18:36Z hyunghwan.chung $
 *
    Copyright (c) 2006-2019 Chung, Hyung-Hwan. All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions
    are met:
    1. Redistributions of source code must retain the above copyright
       notice, this list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright
       notice, this list of conditions and the following disclaimer in the
       documentation and/or other materials provided with the distribution.

    THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
    IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
    OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
    IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
    INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
    NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
    DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
    THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
    THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _QSE_TYPES_H_
#define _QSE_TYPES_H_

/**\file
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

#if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201112L)
#	define QSE_LANG_C11
#else
#	undef QSE_LANG_C11
#endif

#if defined(EMSCRIPTEN)
#	if defined(QSE_SIZEOF___INT128)
#		undef QSE_SIZEOF___INT128 
#		define QSE_SIZEOF___INT128 0
#	endif
#	if defined(QSE_SIZEOF_LONG) && defined(QSE_SIZEOF_INT) && (QSE_SIZEOF_LONG > QSE_SIZEOF_INT)
		/* autoconf doesn't seem to match actual emscripten */
#		undef QSE_SIZEOF_LONG
#		define QSE_SIZEOF_LONG QSE_SIZEOF_INT
#	endif
#endif

/* =========================================================================
 * STATIC ASSERTION
 * =========================================================================*/
#if defined(__GNUC__) && (__GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ > 4))
#	define QSE_UNUSED __attribute__((__unused__))
#else
#	define QSE_UNUSED
#endif

#define QSE_STATIC_JOIN_INNER(x, y) x ## y
#define QSE_STATIC_JOIN(x, y) QSE_STATIC_JOIN_INNER(x, y)

#if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201112L)
#	define QSE_STATIC_ASSERT(expr)  _Static_assert (expr, "invalid assertion")
#elif defined(__cplusplus) && (__cplusplus >= 201103L)
#	define QSE_STATIC_ASSERT(expr) static_assert (expr, "invalid assertion")
#else
#	define QSE_STATIC_ASSERT(expr) typedef char QSE_STATIC_JOIN(QSE_STATIC_ASSERT_T_, __LINE__)[(expr)? 1: -1] QSE_UNUSED
#endif

#define QSE_STATIC_ASSERT_EXPR(expr) ((void)QSE_SIZEOF(char[(expr)? 1: -1]))

/* =========================================================================
 * BASIC TYPES
 * =========================================================================*/

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

/** \typedef qse_int_t
 * The qse_int_t type defines a signed integer type as large as a pointer.
 */
/** \typedef qse_uint_t
 * The qse_uint_t type defines an unsigned integer type as large as a pointer.
 */
#if (defined(hpux) || defined(__hpux) || defined(__hpux__) || \
     (defined(__APPLE__) && defined(__MACH__))) && \
    (QSE_SIZEOF_VOID_P == QSE_SIZEOF_LONG)
	typedef signed long qse_int_t;
	typedef unsigned long qse_uint_t;
	#define QSE_SIZEOF_INT_T QSE_SIZEOF_LONG
	#define QSE_SIZEOF_UINT_T QSE_SIZEOF_LONG
#elif defined(__SPU__) && (QSE_SIZEOF_VOID_P == QSE_SIZEOF_LONG)
	typedef signed long qse_int_t;
	typedef unsigned long qse_uint_t;
	#define QSE_SIZEOF_INT_T QSE_SIZEOF_LONG
	#define QSE_SIZEOF_UINT_T QSE_SIZEOF_LONG
#elif QSE_SIZEOF_VOID_P == QSE_SIZEOF_INT
	typedef signed int qse_int_t;
	typedef unsigned int qse_uint_t;
	#define QSE_SIZEOF_INT_T QSE_SIZEOF_INT
	#define QSE_SIZEOF_UINT_T QSE_SIZEOF_INT
#elif QSE_SIZEOF_VOID_P == QSE_SIZEOF_LONG
	typedef signed long qse_int_t;
	typedef unsigned long qse_uint_t;
	#define QSE_SIZEOF_INT_T QSE_SIZEOF_LONG
	#define QSE_SIZEOF_UINT_T QSE_SIZEOF_LONG
#elif QSE_SIZEOF_VOID_P == QSE_SIZEOF_LONG_LONG
	typedef signed long long qse_int_t;
	typedef unsigned long long qse_uint_t;
	#define QSE_SIZEOF_INT_T QSE_SIZEOF_LONG_LONG
	#define QSE_SIZEOF_UINT_T QSE_SIZEOF_LONG_LONG
#elif QSE_SIZEOF_VOID_P == QSE_SIZEOF___INT32
	typedef signed __int32 qse_int_t;
	typedef unsigned __int32 qse_uint_t;
	#define QSE_SIZEOF_INT_T QSE_SIZEOF___INT32
	#define QSE_SIZEOF_UINT_T QSE_SIZEOF___INT32
#elif QSE_SIZEOF_VOID_P == QSE_SIZEOF___INT32_T
	typedef __int32_t qse_int_t;
	typedef __uint32_t qse_uint_t;
	#define QSE_SIZEOF_INT_T QSE_SIZEOF___INT32_T
	#define QSE_SIZEOF_UINT_T QSE_SIZEOF___INT32_T
#elif QSE_SIZEOF_VOID_P == QSE_SIZEOF___INT64
	typedef signed __int64 qse_int_t;
	typedef unsigned __int64 qse_uint_t;
	#define QSE_SIZEOF_INT_T QSE_SIZEOF___INT64
	#define QSE_SIZEOF_UINT_T QSE_SIZEOF___INT64
#elif QSE_SIZEOF_VOID_P == QSE_SIZEOF___INT64_T
	typedef __int64_t qse_int_t;
	typedef __uint64_t qse_uint_t;
	#define QSE_SIZEOF_INT_T QSE_SIZEOF___INT64_T
	#define QSE_SIZEOF_UINT_T QSE_SIZEOF___INT64_T
#elif QSE_SIZEOF_VOID_P == QSE_SIZEOF___INT128
	typedef signed __int128 qse_int_t;
	typedef unsigned __int128 qse_uint_t;
	#define QSE_SIZEOF_INT_T QSE_SIZEOF___INT128
	#define QSE_SIZEOF_UINT_T QSE_SIZEOF___INT128
#elif QSE_SIZEOF_VOID_P == QSE_SIZEOF___INT128_T
	typedef __int128_t qse_int_t;
	typedef __uint128_t qse_uint_t;
	#define QSE_SIZEOF_INT_T QSE_SIZEOF___INT128_T
	#define QSE_SIZEOF_UINT_T QSE_SIZEOF___INT128_T
#else
#	error unsupported pointer size
#endif

/** \typedef qse_long_t
 * The qse_long_t type defines the largest signed integer type that supported. 
 */
/** \typedef qse_ulong_t
 * The qse_ulong_t type defines the largest unsigned integer type supported.
 */
#if QSE_SIZEOF_LONG >= QSE_SIZEOF_LONG_LONG
	typedef signed long qse_long_t;
	typedef unsigned long qse_ulong_t;
	#define QSE_SIZEOF_LONG_T QSE_SIZEOF_LONG
	#define QSE_SIZEOF_ULONG_T QSE_SIZEOF_LONG
#elif QSE_SIZEOF_LONG_LONG > 0
	typedef signed long long qse_long_t;
	typedef unsigned long long qse_ulong_t;
	#define QSE_SIZEOF_LONG_T QSE_SIZEOF_LONG_LONG
	#define QSE_SIZEOF_ULONG_T QSE_SIZEOF_LONG_LONG
#elif QSE_SIZEOF___INT64 > 0
	typedef signed __int64 qse_long_t;
	typedef unsigned __int64 qse_ulong_t;
	#define QSE_SIZEOF_LONG_T QSE_SIZEOF___INT64
	#define QSE_SIZEOF_ULONG_T QSE_SIZEOF___INT64
#elif QSE_SIZEOF___INT64_T > 0
	typedef __int64_t qse_long_t;
	typedef __uint64_t qse_ulong_t;
	#define QSE_SIZEOF_LONG_T QSE_SIZEOF___INT64_T
	#define QSE_SIZEOF_ULONG_T QSE_SIZEOF___INT64_T
#else
	typedef signed long qse_long_t;
	typedef unsigned long qse_ulong_t;
	#define QSE_SIZEOF_LONG_T QSE_SIZEOF_LONG
	#define QSE_SIZEOF_ULONG_T QSE_SIZEOF_LONG
#endif

/* these two items are revised whenever the size of a 
 * fixed-size integer is determined */
#define QSE_SIZEOF_INTMAX_T  0
#define QSE_SIZEOF_UINTMAX_T 0

/** \typedef qse_int8_t
 * The qse_int8_t defines an 8-bit signed integer type.
 */
/** \typedef qse_uint8_t
 * The qse_uint8_t type defines an 8-bit unsigned integer type.
 */
#if QSE_SIZEOF_CHAR == 1
#	define QSE_HAVE_INT8_T
#	define QSE_HAVE_UINT8_T
	typedef signed char qse_int8_t;
	typedef unsigned char qse_uint8_t;
#elif QSE_SIZEOF___INT8 == 1
#	define QSE_HAVE_INT8_T
#	define QSE_HAVE_UINT8_T
	typedef signed __int8 qse_int8_t;
	typedef unsigned __int8 qse_uint8_t;
#elif QSE_SIZEOF___INT8_T == 1
#	define QSE_HAVE_INT8_T
#	define QSE_HAVE_UINT8_T
	typedef __int8_t qse_int8_t;
	typedef __uint8_t qse_uint8_t;
#endif

#if defined(QSE_HAVE_INT8_T)
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

/** \typedef qse_int16_t
 * The qse_int16_t defines an 16-bit signed integer type.
 */
/** \typedef qse_uint16_t
 * The qse_uint16_t type defines an 16-bit unsigned integer type.
 */
#if QSE_SIZEOF_SHORT == 2
#	define QSE_HAVE_INT16_T
#	define QSE_HAVE_UINT16_T
	typedef signed short qse_int16_t;
	typedef unsigned short qse_uint16_t;
#elif QSE_SIZEOF___INT16 == 2
#	define QSE_HAVE_INT16_T
#	define QSE_HAVE_UINT16_T
	typedef signed __int16 qse_int16_t;
	typedef unsigned __int16 qse_uint16_t;
#elif QSE_SIZEOF___INT16_T == 2
#	define QSE_HAVE_INT16_T
#	define QSE_HAVE_UINT16_T
	typedef __int16_t qse_int16_t;
	typedef __uint16_t qse_uint16_t;
#endif

#if defined(QSE_HAVE_INT16_T)
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

/** \typedef qse_int32_t
 * The qse_int32_t defines an 32-bit signed integer type.
 */
/** \typedef qse_uint32_t
 * The qse_uint32_t type defines an 32-bit unsigned integer type.
 */
#if QSE_SIZEOF_INT == 4
#	define QSE_HAVE_INT32_T
#	define QSE_HAVE_UINT32_T
	typedef signed int qse_int32_t;
	typedef unsigned int qse_uint32_t;
#elif QSE_SIZEOF_LONG == 4
#	define QSE_HAVE_INT32_T
#	define QSE_HAVE_UINT32_T
	typedef signed long qse_int32_t;
	typedef unsigned long qse_uint32_t;
#elif QSE_SIZEOF___INT32 == 4
#	define QSE_HAVE_INT32_T
#	define QSE_HAVE_UINT32_T
	typedef signed __int32 qse_int32_t;
	typedef unsigned __int32 qse_uint32_t;
#elif QSE_SIZEOF___INT32_T == 4
#	define QSE_HAVE_INT32_T
#	define QSE_HAVE_UINT32_T
	typedef __int32_t qse_int32_t;
	typedef __uint32_t qse_uint32_t;
#endif

#if defined(QSE_HAVE_INT32_T)
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

/** \typedef qse_int64_t
 * The qse_int64_t defines an 64-bit signed integer type.
 */
/** \typedef qse_uint64_t
 * The qse_uint64_t type defines an 64-bit unsigned integer type.
 */
#if QSE_SIZEOF_INT == 8
#	define QSE_HAVE_INT64_T
#	define QSE_HAVE_UINT64_T
	typedef signed int qse_int64_t;
	typedef unsigned int qse_uint64_t;
#elif QSE_SIZEOF_LONG == 8
#	define QSE_HAVE_INT64_T
#	define QSE_HAVE_UINT64_T
	typedef signed long qse_int64_t;
	typedef unsigned long qse_uint64_t;
#elif QSE_SIZEOF_LONG_LONG == 8
#	define QSE_HAVE_INT64_T
#	define QSE_HAVE_UINT64_T
	typedef signed long long qse_int64_t;
	typedef unsigned long long qse_uint64_t;
#elif QSE_SIZEOF___INT64 == 8
#	define QSE_HAVE_INT64_T
#	define QSE_HAVE_UINT64_T
	typedef signed __int64 qse_int64_t;
	typedef unsigned __int64 qse_uint64_t;
#elif QSE_SIZEOF___INT64_T == 8
#	define QSE_HAVE_INT64_T
#	define QSE_HAVE_UINT64_T
	typedef __int64_t qse_int64_t;
	typedef __uint64_t qse_uint64_t;
#endif

#if defined(QSE_HAVE_INT64_T)
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
	typedef signed int qse_int128_t;
	typedef unsigned int qse_uint128_t;
#elif QSE_SIZEOF_LONG == 16
#	define QSE_HAVE_INT128_T
#	define QSE_HAVE_UINT128_T
	typedef signed long qse_int128_t;
	typedef unsigned long qse_uint128_t;
#elif QSE_SIZEOF_LONG_LONG == 16
#	define QSE_HAVE_INT128_T
#	define QSE_HAVE_UINT128_T
	typedef signed long long qse_int128_t;
	typedef unsigned long long qse_uint128_t;
#elif QSE_SIZEOF___INT128 == 16
#	define QSE_HAVE_INT128_T
#	define QSE_HAVE_UINT128_T
	typedef signed __int128 qse_int128_t;
	typedef unsigned __int128 qse_uint128_t;
#elif (QSE_SIZEOF___INT128_T == 16)
#	define QSE_HAVE_INT128_T
#	define QSE_HAVE_UINT128_T
	typedef __int128_t qse_int128_t;
	typedef __uint128_t qse_uint128_t;
#endif

#if defined(QSE_HAVE_INT128_T)
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

/** \typedef qse_intmax_t
 * The qse_llong_t type defines the largest signed integer type supported.
 */
/** \typedef qse_uintmax_t
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


/** \typedef qse_flt_t
 * The qse_flt_t type defines the largest floating-pointer number type
 * naturally supported.
 */
#if defined(__FreeBSD__) || defined(__MINGW32__)
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

/** \typedef qse_fltmax_t
 * The qse_fltmax_t type defines the largest floating-pointer number type
 * ever supported.
 */
#if QSE_SIZEOF___FLOAT128 >= QSE_SIZEOF_FLT_T
	/* the size of long double may be equal to the size of __float128
	 * for alignment on some platforms */
	typedef __float128 qse_fltmax_t;
#	define QSE_SIZEOF_FLTMAX_T QSE_SIZEOF___FLOAT128
#	define QSE_FLTMAX_REQUIRE_QUADMATH 1
#else
	typedef qse_flt_t qse_fltmax_t;
#	define QSE_SIZEOF_FLTMAX_T QSE_SIZEOF_FLT_T
#	undef QSE_FLTMAX_REQUIRE_QUADMATH
#endif

/** \typedef qse_ptrdiff_t
 */
typedef qse_ssize_t qse_ptrdiff_t;
#define QSE_SIZEOF_PTRDIFF_T QSE_SIZEOF_SSIZE_T

/** 
 * The qse_mchar_t type defines a multi-byte character type.
 */
typedef char qse_mchar_t;
#define QSE_SIZEOF_MCHAR_T QSE_SIZEOF_CHAR 

/** \typedef qse_mchau_t
 * The qse_mchau_t type defines a type that can hold the unsigned qse_mchar_t value.
 */
typedef unsigned char qse_mchau_t;

/**
 * The qse_mcint_t defines a type that can hold a qse_mchar_t value and 
 * #QSE_MCHAR_EOF.
 */
typedef int qse_mcint_t;
/*#define QSE_SIZEOF_MCINT_T QSE_SIZEOF_INT*/


/** \typedef qse_wchar_t
 * The qse_wchar_t type defines a wide character type. 
 */
/** \typedef qse_wchau_t
 * The qse_wchau_t type defines a type that can hold the unsigned qse_wchar_t value.
 */
/** \typedef qse_wcint_t
 * The qse_wcint_t type defines a type that can hold a qse_wchar_t value and 
 * #QSE_WCHAR_EOF.
 */

// You may specify -DQSE_USE_CXX_CHAR16_T in CXXFLAGS to force char16_t with c++.
#if (defined(__cplusplus) && defined(QSE_USE_CXX_CHAR16_T) && ((__cplusplus >= 201103L) || (defined(_MSC_VER) && _MSC_VER >= 1900)) && defined(QSE_WIDE_CHAR_SIZE) && (QSE_WIDE_CHAR_SIZE == 4))
		typedef char32_t           qse_wchar_t;  /* char32_t is an unsigned integer type used for 32-bit wide characters */
		typedef char32_t           qse_wchau_t; /* same as qse_wchar_t as it is already unsigned */
#		define QSE_SIZEOF_WCHAR_T 4
#		define QSE_USE_PREFIX_BIG_U

#elif (defined(__cplusplus) && defined(QSE_USE_CXX_CHAR32_T) && ((__cplusplus >= 201103L) || (defined(_MSC_VER) && _MSC_VER >= 1900))  && defined(QSE_WIDE_CHAR_SIZE) && (QSE_WIDE_CHAR_SIZE == 2))
		typedef char16_t           qse_wchar_t;  /* char16_t is an unsigned integer type used for 16-bit wide characters */
		typedef char16_t           qse_wchau_t; /* same as qse_wchar_t as it is already unsigned */
#		define QSE_SIZEOF_WCHAR_T 2
#		define QSE_USE_PREFIX_SMALL_U
#elif defined(__cplusplus) && defined(QSE_WIDE_CHAR_SIZE) && (QSE_WIDE_CHAR_SIZE >= 4) && (QSE_SIZEOF_NATIVE_WCHAR_T >= 4)
	typedef wchar_t           qse_wchar_t;
	typedef qse_uint32_t      qse_wchau_t;
#	define QSE_SIZEOF_WCHAR_T 4

	// if this assertion becomes false, you must check if the size of the wchar_t type is the same as the size used
	// for this library.
	QSE_STATIC_ASSERT (QSE_WIDE_CHAR_SIZE == sizeof(qse_wchar_t));

#elif defined(__cplusplus) && defined(QSE_WIDE_CHAR_SIZE) && (QSE_WIDE_CHAR_SIZE == 2) && (QSE_SIZEOF_NATIVE_WCHAR_T == 2)
	typedef wchar_t           qse_wchar_t;
	typedef qse_uint16_t      qse_wchau_t;
#	define QSE_SIZEOF_WCHAR_T 2

	// if the library is compiled with 2-byte wchar_t, and the library user compiles a program with 4-byte wchar_t,
	// there will be size disparity issue on the qse_wchar_t type.
	// if this assertion becomes false, you must check if the size of the wchar_t type is the same as the size used 
	// for this library.
	//   gcc/g++/clang/clang++: -fshort-wchar makes wchar_t to 2 bytes.
	QSE_STATIC_ASSERT (QSE_WIDE_CHAR_SIZE == sizeof(qse_wchar_t));

#elif defined(QSE_WIDE_CHAR_SIZE) && (QSE_WIDE_CHAR_SIZE >= 4) && defined(__CHAR32_TYPE__) && defined(QSE_HAVE_PREFIX_BIG_U)
	typedef __CHAR32_TYPE__    qse_wchar_t;
	typedef qse_uint32_t        qse_wchau_t;
#	define QSE_SIZEOF_WCHAR_T 4
#	if !defined(QSE_PREFER_PREFIX_L)
#	define QSE_USE_PREFIX_BIG_U
#	endif

#elif defined(QSE_WIDE_CHAR_SIZE) && (QSE_WIDE_CHAR_SIZE >= 4)
	typedef qse_uint32_t      qse_wchar_t;
	typedef qse_uint32_t      qse_wchau_t;
#	define QSE_SIZEOF_WCHAR_T 4

#elif defined(__CHAR16_TYPE__) && defined(QSE_HAVE_PREFIX_SMALL_U)
	typedef __CHAR16_TYPE__    qse_wchar_t;
	typedef qse_uint16_t       qse_wchau_t;
#	define QSE_SIZEOF_WCHAR_T 2
#	if !defined(QSE_PREFER_PREFIX_L)
#	define QSE_USE_PREFIX_SMALL_U
#	endif

#else
	typedef qse_uint16_t      qse_wchar_t;
	typedef qse_uint16_t      qse_wchau_t; /* same as qse_wchar_t as it is already unsigned */
#	define QSE_SIZEOF_WCHAR_T 2
#endif

typedef qse_int32_t qse_wcint_t;
#define QSE_SIZEOF_WCINT_T 4


/** \typedef qse_char_t
 * The qse_char_t type defines a character type.
 */
/** \typedef qse_chau_t
 * The qse_chau_t type defines a type that can hold a unsigned qse_char_t value.
 */
/** \typedef qse_cint_t
 * The qse_cint_t type defines a type that can hold a qse_char_t value and 
 * #QSE_CHAR_EOF.
 */
#if defined(QSE_ENABLE_WIDE_CHAR)
#	define QSE_CHAR_IS_WCHAR
#	define QSE_SIZEOF_CHAR_T QSE_SIZEOF_WCHAR_T
	typedef qse_wchar_t qse_char_t;
	typedef qse_wchau_t qse_chau_t;
	typedef qse_wcint_t qse_cint_t;

#else
#	define QSE_CHAR_IS_MCHAR
#	define QSE_SIZEOF_CHAR_T QSE_SIZEOF_MCHAR_T
	typedef qse_mchar_t qse_char_t;
	typedef qse_mchau_t qse_chau_t;
	typedef qse_mcint_t qse_cint_t;
#endif


#if 0

	/* If the character type is not determined in the conf_xxx files */

#	if defined(_WIN32)
#		if defined(UNICODE) || defined(_UNICODE)
#			define QSE_CHAR_IS_WCHAR
#			define QSE_SIZEOF_CHAR_T QSE_SIZEOF_WCHAR_T
			typedef qse_wchar_t qse_char_t;
			typedef qse_wchau_t qse_chau_t;
			typedef qse_wcint_t qse_cint_t;
#		else
#			define QSE_CHAR_IS_MCHAR
#			define QSE_SIZEOF_CHAR_T QSE_SIZEOF_MCHAR_T
			typedef qse_mchar_t qse_char_t;
			typedef qse_mchau_t qse_chau_t;
			typedef qse_mcint_t qse_cint_t;
#		endif
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
 * The qse_mcstr_t type defines a structure combining a pointer to a character
 * string and the number of characters. 
 */
struct qse_mcstr_t
{
	qse_mchar_t* ptr; /**< pointer to a character string */
	qse_size_t   len; /**< the number of characters */
};
typedef struct qse_mcstr_t qse_mcstr_t;

/**
 * The qse_wcstr_t type defines a structure combining a pointer to a character
 * string and the number of characters.
 */
struct qse_wcstr_t
{
	qse_wchar_t* ptr; /**< pointer to a character string */
	qse_size_t   len; /**< the number of characters */
};
typedef struct qse_wcstr_t qse_wcstr_t;

#if defined(QSE_CHAR_IS_MCHAR)
	typedef qse_mcstr_t qse_cstr_t;
#else
	typedef qse_wcstr_t qse_cstr_t;
#endif


/**
 * The qse_u8ptl_t type defines a structure with a pointer to 8-bit unsigned
 * integer string and its length.
 */
struct qse_u8ptl_t
{
	qse_uint8_t* ptr;
	qse_size_t   len;
};
typedef struct qse_u8ptl_t qse_u8ptl_t;

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
 * The qse_ioptl_t type defines an analogus type to 'struct iovec' typically
 * found on posix platforms
 */
#if (QSE_SIZEOF_STRUCT_IOVEC > 0) && (QSE_OFFSETOF_IOV_BASE != QSE_OFFSETOF_IOV_LEN)
	struct qse_ioptl_t
	{
	/* [THINK] do i have to go extreme to inject fillers by looking at the total size and the offsets ? 
	 *         i believe most systems should define only two members - iovec_base and iovec_len */
	#if (QSE_OFFSETOF_IOV_BASE < QSE_OFFSETOF_IOV_LEN)
		void*      ptr;
		qse_size_t len;
	#else
		qse_size_t len;
		void*      ptr;
	#endif
	};
	typedef struct qse_ioptl_t qse_ioptl_t;
#else
	typedef qse_xptl_t qse_ioptl_t;
#endif


/**
 * The qse_floc_t type defines a structure that can hold a position
 * in a file.
 */
struct qse_floc_t
{
	const qse_char_t* file;
	qse_size_t        line;
	qse_size_t        colm;
};
typedef struct qse_floc_t qse_floc_t;

#if defined(__cplusplus)
struct qse_flocxx_t: qse_floc_t
{
	qse_flocxx_t () 
	{
		this->file = (const qse_char_t*)0 /*QSE_NULL*/;
		this->line = 0;
		this->colm = 0;
	}

	qse_flocxx_t (qse_size_t line, qse_size_t colm)
	{
		this->file = (const qse_char_t*)0 /*QSE_NULL*/;
		this->line = line;
		this->colm = colm;
	}

	qse_flocxx_t& operator= (const qse_floc_t& floc)
	{
		this->file = floc.file;
		this->line = floc.line;
		this->colm = floc.colm;
		return *this;
	}
};
#endif

typedef struct qse_mmgr_t qse_mmgr_t;

/** 
 * allocate a memory chunk of the size \a n.
 * \return pointer to a memory chunk on success, QSE_NULL on failure.
 */
typedef void* (*qse_mmgr_alloc_t)   (qse_mmgr_t* mmgr, qse_size_t n);
/** 
 * resize a memory chunk pointed to by \a ptr to the size \a n.
 * \return pointer to a memory chunk on success, QSE_NULL on failure.
 */
typedef void* (*qse_mmgr_realloc_t) (qse_mmgr_t* mmgr, void* ptr, qse_size_t n);
/**
 * free a memory chunk pointed to by \a ptr.
 */
typedef void  (*qse_mmgr_free_t)    (qse_mmgr_t* mmgr, void* ptr);

/**
 * The qse_mmgr_t type defines the memory management interface.
 * As the type is merely a structure, it is just used as a single container
 * for memory management functions with a pointer to user-defined data. 
 * The user-defined data pointer \a ctx is passed to each memory management 
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
#elif defined(QSE_HAVE_INT8_T) && (QSE_SIZEOF_OFF_T==1)
	typedef qse_int8_t qse_foff_t;
#	define QSE_SIZEOF_FOFF_T QSE_SIZEOF_INT16_T
#else
	typedef qse_int32_t qse_foff_t; /* this line is for doxygen */
#	error Unsupported platform
#endif

/**
 * The #qse_fmode_t type defines an integer that can represent a file offset.
 * Depending on your system, it's defined to one of #qse_int64_t, #qse_int32_t,
 * and #qse_int16_t.
 */
#if defined(QSE_MODE_T_IS_SIGNED)
#	if defined(QSE_HAVE_INT64_T) && (QSE_SIZEOF_MODE_T==8)
		typedef qse_int64_t qse_fmode_t;
#		define QSE_SIZEOF_FMODE_T QSE_SIZEOF_INT64_T
#	elif defined(QSE_HAVE_INT32_T) && (QSE_SIZEOF_MODE_T==4)
		typedef qse_int32_t qse_fmode_t;
#		define QSE_SIZEOF_FMODE_T QSE_SIZEOF_INT32_T
#	elif defined(QSE_HAVE_INT16_T) && (QSE_SIZEOF_MODE_T==2)
		typedef qse_int16_t qse_fmode_t;
#		define QSE_SIZEOF_FMODE_T QSE_SIZEOF_INT16_T
#	elif defined(QSE_HAVE_INT8_T) && (QSE_SIZEOF_MODE_T==1)
		typedef qse_int8_t qse_fmode_t;
#		define QSE_SIZEOF_FMODE_T QSE_SIZEOF_INT8_T
#	else
		typedef qse_int32_t qse_fmode_t; /* this line is for doxygen */
#		error Unsupported platform
#	endif
#else
#	if defined(QSE_HAVE_INT64_T) && (QSE_SIZEOF_MODE_T==8)
		typedef qse_uint64_t qse_fmode_t;
#		define QSE_SIZEOF_FMODE_T QSE_SIZEOF_INT64_T
#	elif defined(QSE_HAVE_INT32_T) && (QSE_SIZEOF_MODE_T==4)
		typedef qse_uint32_t qse_fmode_t;
#		define QSE_SIZEOF_FMODE_T QSE_SIZEOF_INT32_T
#	elif defined(QSE_HAVE_INT16_T) && (QSE_SIZEOF_MODE_T==2)
		typedef qse_uint16_t qse_fmode_t;
#		define QSE_SIZEOF_FMODE_T QSE_SIZEOF_INT16_T
#	elif defined(QSE_HAVE_INT8_T) && (QSE_SIZEOF_MODE_T==1)
		typedef qse_uint8_t qse_fmode_t;
#		define QSE_SIZEOF_FMODE_T QSE_SIZEOF_INT8_T
#	else
		typedef qse_uint32_t qse_fmode_t; /* this line is for doxygen */
#		error Unsupported platform
#	endif
#endif

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
