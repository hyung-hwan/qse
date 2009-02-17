/*
 * $Id: types.h 476 2008-12-12 06:25:48Z baconevi $
 *
   Copyright 2006-2009 Chung, Hyung-Hwan.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
 */

#ifndef _QSE_TYPES_H_
#define _QSE_TYPES_H_

/****o* Base/Basic Types
 * DESCRIPTION
 *  <qse/types.h> defines various common basic types. They are designed to be
 *  cross-platform. These types are preferred over native data types in many
 *  contexts.
 *
 *  #include <qse/types.h>
 ******
 */

/* WARNING: NEVER CHANGE/DELETE THE FOLLOWING LINE */
/*#define QSE_HAVE_CONFIG_H*/

#if defined(QSE_HAVE_CONFIG_H)
#	include <qse/config.h>
#elif defined(_WIN32)
#	include <qse/conf_msw.h>
#elif defined(vms) || defined(__vms)
#	include <qse/conf_vms.h>
#else
#	error unsupported operating system
#endif

/****t* Base/qse_bool_t
 * NAME
 *  qse_bool_t - define a boolean type
 * DESCRIPTION
 *  The qse_bool_t type defines a boolean type that can represent QSE_TRUE 
 *  and QSE_FALSE.
 * SYNOPSIS
 */
enum qse_bool_t
{
	QSE_TRUE  = (0 == 0),
	QSE_FALSE = (0 != 0)
};
/******/
typedef enum qse_bool_t qse_bool_t;

/****t* Base/qse_tri_t
 * NAME
 *  qse_tri_t - define a tri-state type
 * DESCRIPTION
 *  The qse_tri_t type defines a tri-state type that can represent QSE_ALIVE,
 *  QSE_ZOMBIE, and QSE_DEAD.
 * SYNOPSIS
 */
enum qse_tri_t
{
	QSE_ALIVE  = 1,
	QSE_ZOMBIE = 0,
	QSE_DEAD   = -1
};
/******/
typedef enum qse_tri_t qse_tri_t;

/****t* Base/qse_int_t,Base/qse_uint_t
 * NAME
 *  * qse_int_t - define a signed integer type as large as a pointer type
 *  * qse_uint_t - define an unsigned integer type as large as a pointer type
 ******
 */
#if (defined(hpux) || defined(__hpux) || defined(__hpux__)) && \
    (QSE_SIZEOF_VOID_P == QSE_SIZEOF_LONG)
	typedef long qse_int_t;
	typedef unsigned long qse_uint_t;
#elif defined(__SPU__) && (QSE_SIZEOF_VOID_P == QSE_SIZEOF_LONG)
	typedef long qse_int_t;
	typedef unsigned long qse_uint_t;
#elif QSE_SIZEOF_VOID_P == QSE_SIZEOF_INT
	typedef int qse_int_t;
	typedef unsigned int qse_uint_t;
#elif QSE_SIZEOF_VOID_P == QSE_SIZEOF_LONG
	typedef long qse_int_t;
	typedef unsigned long qse_uint_t;
#elif QSE_SIZEOF_VOID_P == QSE_SIZEOF_LONG_LONG
	typedef long long qse_int_t;
	typedef unsigned long long qse_uint_t;
#elif QSE_SIZEOF_VOID_P == QSE_SIZEOF___INT32
	typedef __int32 qse_int_t;
	typedef unsigned __int32 qse_uint_t;
#elif QSE_SIZEOF_VOID_P == QSE_SIZEOF___INT64
	typedef __int64 qse_int_t;
	typedef unsigned __int64 qse_uint_t;
#else
#	error unsupported pointer size
#endif

/****t* Base/qse_long_t,Base/qse_ulong_t
 * NAME
 *  * qse_long_t - define the largest signed integer type supported
 *  * qse_ulong_t - define the largest unsigned integer type supported
 ******
 */
#if QSE_SIZEOF_LONG_LONG > 0
	typedef long long qse_long_t;
	typedef unsigned long long qse_ulong_t;
#elif QSE_SIZEOF___INT64 > 0
	typedef __int64 qse_long_t;
	typedef unsigned __int64 qse_ulong_t;
#else
	typedef long qse_long_t;
	typedef unsigned long qse_ulong_t;
#endif

/****t* Base/qse_int8_t,Base/qse_uint8_t
 * NAME
 *  * qse_int8_t - define an 8-bit signed integer type
 *  * qse_uint8_t - define an 8-bit unsigned integer type
 ******
 */
#if QSE_SIZEOF_CHAR == 1
	typedef char qse_int8_t;
	typedef unsigned char qse_uint8_t;
#elif QSE_SIZEOF___INT8 == 1
	typedef __int8 qse_int8_t;
	typedef unsigned __int8 qse_uint8_t;
#endif

/****t* Base/qse_int16_t,Base/qse_uint16_t
 * NAME
 *  * qse_int16_t - define a 16-bit signed integer type
 *  * qse_uint16_t - define a 16-bit unsigned integer type
 ******
 */
#if QSE_SIZEOF_SHORT == 2
	typedef short qse_int16_t;
	typedef unsigned short qse_uint16_t;
#elif QSE_SIZEOF___INT16 == 2
	typedef __int16 qse_int16_t;
	typedef unsigned __int16 qse_uint16_t;
#endif

/****t* Base/qse_int32_t,Base/qse_uint32_t
 * NAME
 *  * qse_int32_t - define a 32-bit signed integer type
 *  * qse_uint32_t - define a 32-bit unsigned integer type
 ******
 */
#if QSE_SIZEOF_INT == 4
	typedef int qse_int32_t;
	typedef unsigned int qse_uint32_t;
#elif QSE_SIZEOF_LONG == 4
	typedef long qse_int32_t;
	typedef unsigned long qse_uint32_t;
#elif QSE_SIZEOF___INT32 == 4
	typedef __int32 qse_int32_t;
	typedef unsigned __int32 qse_uint32_t;
#endif

/****t* Base/qse_int64_t,Base/qse_uint64_t
 * NAME
 *  * qse_int64_t - define a 64-bit signed integer type
 *  * qse_uint64_t - define a 64-bit unsigned integer type
 ******
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
#endif

/****t* Base/qse_byte_t
 * NAME
 *  qse_byte_t - define a byte type
 ******
 */
typedef qse_uint8_t qse_byte_t;

/****t* Base/qse_size_t
 * NAME
 *  qse_size_t - define an unsigned integer type that can hold a pointer value
 ******
 */
#ifdef __SIZE_TYPE__
	typedef __SIZE_TYPE__ qse_size_t;
#else
	typedef qse_uint_t  qse_size_t;
#endif

/****t* Base/qse_ssize_t
 * NAME
 *  qse_ssize_t - define an signed integer type that can hold a pointer value
 ******
 */
typedef qse_int_t   qse_ssize_t;

/****t* Base/qse_word_t
 * NAME
 *  qse_word_t - define an integer type identical to qse_uint_t 
 ******
 */
typedef qse_uint_t  qse_word_t;

/* floating-point number */
#if defined(__FreeBSD__)
	/* TODO: check if the support for long double is complete.
	 *       if so, use long double for qse_real_t */
#	define QSE_SIZEOF_REAL QSE_SIZEOF_DOUBLE
	typedef double qse_real_t;
#elif QSE_SIZEOF_LONG_DOUBLE > QSE_SIZEOF_DOUBLE
#	define QSE_SIZEOF_REAL QSE_SIZEOF_LONG_DOUBLE
	typedef long double qse_real_t;
#else
#	define QSE_SIZEOF_REAL QSE_SIZEOF_DOUBLE
	typedef double qse_real_t;
#endif

/****t* Base/qse_mchar_t,Base/qse_mcint_t
 * NAME
 *  * qse_mchar_t - define a multi-byte character
 *  * qse_mcint_t - define a type that can hold qse_mchar_t and QSE_MCHAR_EOF
 ******
 */
typedef char qse_mchar_t;
typedef int  qse_mcint_t;

/****t* Base/qse_wchar_t,Base/qse_wcint_t
 * NAME
 *  * qse_wchar_t - define a wide character
 *  * qse_wcint_t - define a type that can hold qse_wchar_t and QSE_WCHAR_EOF
 ******
 */
#if defined(__cplusplus) && \
    (!defined(_MSC_VER) || \
     (defined(_MSC_VER) && defined(_NATIVE_WCHAR_T_DEFINED)))
	/* C++ */

	typedef wchar_t qse_wchar_t;
	typedef wchar_t qse_wcint_t;

	/* all the way down from here for C */
#elif defined(__WCHAR_TYPE__) && defined(__WINT_TYPE__)
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
#		error no supported data type for wchar_t
#	endif
#else
#	error unsupported size of wchar_t
#endif

/****t* Base/qse_char_t,Base/qse_cint_t
 * NAME
 *  * qse_char_t - define a character
 *  * qse_cint_t - define a type that can hold qse_char_t and QSE_CHAR_EOF
 ******
 */
#if defined(_WIN32) && (defined(UNICODE)||defined(_UNICODE))
#	define QSE_CHAR_IS_WCHAR
	typedef qse_wchar_t qse_char_t;
	typedef qse_wcint_t qse_cint_t;
#else
#	if defined(QSE_CHAR_IS_MCHAR)
		typedef qse_mchar_t qse_char_t;
		typedef qse_mcint_t qse_cint_t;
#	elif defined(QSE_CHAR_IS_WCHAR)
		typedef qse_wchar_t qse_char_t;
		typedef qse_wcint_t qse_cint_t;
#	elif defined(_MBCS)
#		define QSE_CHAR_IS_MCHAR
		typedef qse_mchar_t qse_char_t;
		typedef qse_mcint_t qse_cint_t;
#	else
#		define QSE_CHAR_IS_WCHAR
		typedef qse_wchar_t qse_char_t;
		typedef qse_wcint_t qse_cint_t;
#	endif
#endif

#if defined(QSE_CHAR_IS_WCHAR) && defined(_WIN32) 
#	ifndef UNICODE
#		define UNICODE
#	endif
#	ifndef _UNICODE
#		define _UNICODE
#	endif
#endif

typedef struct qse_xstr_t qse_xstr_t;
typedef struct qse_cstr_t qse_cstr_t;
typedef struct qse_mmgr_t qse_mmgr_t;
typedef struct qse_ccls_t qse_ccls_t;

/****t* Base/qse_xstr_t
 * NAME
 *  qse_xstr_t - combine a pointer and length 
 * SEE ALSO
 *  qse_cstr_t
 * SYNOPSIS
 */
struct qse_xstr_t
{
	qse_char_t* ptr; /* this is not a const pointer */
	qse_size_t  len; /* the number of characters */
};
/******/

/****t* Base/qse_cstr_t
 * NAME
 *  qse_cstr_t - combine a constant pointer and length 
 * SEE ALSO
 *  qse_xstr_t
 * SYNOPSIS
 */
struct qse_cstr_t
{
	const qse_char_t* ptr; /* this is a const pointer */
	qse_size_t        len; /* the number of characters */
};
/******/

/****t* Base/qse_mmgr_t
 * NAME
 *  qse_mmgr_t - define a memory manager
 * SYNOPSIS
 */
struct qse_mmgr_t
{
	void* (*alloc)   (void* data, qse_size_t n);
	void* (*realloc) (void* data, void* ptr, qse_size_t n);
	void  (*free)    (void* data, void* ptr);
	void*   data;
};
/******/

/****t* Base/qse_ccls_id_t
 * NAME
 *  qse_ccls_id_t - define character class types
 * SYNOPSIS
 */
enum qse_ccls_id_t
{
        QSE_CCLS_UPPER,
        QSE_CCLS_LOWER,
        QSE_CCLS_ALPHA,
        QSE_CCLS_DIGIT,
        QSE_CCLS_XDIGIT,
        QSE_CCLS_ALNUM,
        QSE_CCLS_SPACE,
        QSE_CCLS_PRINT,
        QSE_CCLS_GRAPH,
        QSE_CCLS_CNTRL,
        QSE_CCLS_PUNCT
};
/******/

typedef enum qse_ccls_id_t qse_ccls_id_t;

typedef qse_bool_t (*qse_ccls_is_t) (
	void* data, qse_cint_t c, qse_ccls_id_t type);
typedef qse_cint_t (*qse_ccls_to_t) (
	void* data, qse_cint_t c, qse_ccls_id_t type);

/****t* Base/qse_ccls_t
 * NAME
 *  qse_mmgr_t - define a character classifier
 * SYNOPSIS
 */
struct qse_ccls_t
{
	qse_ccls_is_t is;
	qse_ccls_to_t to;
	void*         data;
};
/******/

#endif
