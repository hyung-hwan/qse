/*
 * $Id: types.h 230 2009-07-13 08:51:23Z hyunghwan.chung $
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

/**@file
 * <qse/types.h> defines various common basic types designed to be
 * cross-platform. These types are preferred over native data types.
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

/**
 * The qse_tri_t type defines a tri-state type that can represent #QSE_ALIVE,
 * #QSE_ZOMBIE, and #QSE_DEAD.
 */
enum qse_tri_t
{
	QSE_ALIVE  = 1,
	QSE_ZOMBIE = 0,
	QSE_DEAD   = -1
};
typedef enum qse_tri_t qse_tri_t;

/** @typedef qse_int_t
 * The qse_int_t type defines a signed integer type as large as a pointer.
 */
/** @typedef qse_uint_t
 * The qse_uint_t type defines an unsigned integer type as large as a pointer.
 */
#if (defined(hpux) || defined(__hpux) || defined(__hpux__)) && \
    (QSE_SIZEOF_VOID_P == QSE_SIZEOF_LONG)
	typedef long qse_int_t;
	typedef unsigned long qse_uint_t;
	#define QSE_SIZEOF_INT_T QSE_SIZEOF_LONG
#elif defined(__SPU__) && (QSE_SIZEOF_VOID_P == QSE_SIZEOF_LONG)
	typedef long qse_int_t;
	typedef unsigned long qse_uint_t;
	#define QSE_SIZEOF_INT_T QSE_SIZEOF_LONG
#elif QSE_SIZEOF_VOID_P == QSE_SIZEOF_INT
	typedef int qse_int_t;
	typedef unsigned int qse_uint_t;
	#define QSE_SIZEOF_INT_T QSE_SIZEOF_INT
#elif QSE_SIZEOF_VOID_P == QSE_SIZEOF_LONG
	typedef long qse_int_t;
	typedef unsigned long qse_uint_t;
	#define QSE_SIZEOF_INT_T QSE_SIZEOF_LONG
#elif QSE_SIZEOF_VOID_P == QSE_SIZEOF_LONG_LONG
	typedef long long qse_int_t;
	typedef unsigned long long qse_uint_t;
	#define QSE_SIZEOF_INT_T QSE_SIZEOF_LONG_LONG
#elif QSE_SIZEOF_VOID_P == QSE_SIZEOF___INT32
	typedef __int32 qse_int_t;
	typedef unsigned __int32 qse_uint_t;
	#define QSE_SIZEOF_INT_T QSE_SIZEOF___INT32
#elif QSE_SIZEOF_VOID_P == QSE_SIZEOF___INT64
	typedef __int64 qse_int_t;
	typedef unsigned __int64 qse_uint_t;
	#define QSE_SIZEOF_INT_T QSE_SIZEOF___INT64
#else
#	error unsupported pointer size
#endif

/** @typedef qse_long_t
 * The qse_long_t type defines the largest signed integer type supported
 */
/** @typedef qse_ulong_t
 * The qse_ulong_t type defines the largest unsigned integer type supported
 */
#if QSE_SIZEOF_LONG_LONG > 0
	typedef long long qse_long_t;
	typedef unsigned long long qse_ulong_t;
	#define QSE_SIZEOF_LONG_T QSE_SIZEOF_LONG_LONG
#elif QSE_SIZEOF___INT64 > 0
	typedef __int64 qse_long_t;
	typedef unsigned __int64 qse_ulong_t;
	#define QSE_SIZEOF_LONG_T QSE_SIZEOF___INT64
#else
	typedef long qse_long_t;
	typedef unsigned long qse_ulong_t;
	#define QSE_SIZEOF_LONG_T QSE_SIZEOF_LONG
#endif

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

/**
 * The qse_byte_t defines a byte type.
 */
typedef qse_uint8_t qse_byte_t;

/**
 * The qse_size_t type defines an unsigned integer type that is as large as
 * to hold a pointer value.
 */
#ifdef __SIZE_TYPE__
	typedef __SIZE_TYPE__ qse_size_t;
#else
	typedef qse_uint_t  qse_size_t;
#endif

/**
 * The qse_ssize_t type defines a signed integer type that is as large as
 * to hold a pointer value.
 */
typedef qse_int_t qse_ssize_t;

/** 
 * The qse_word_t type redefines qse_uint_t. 
 */
typedef qse_uint_t qse_word_t;

/** @typedef qse_real_t
 * The qse_real_t type defines the largest floating-pointer number type
 * supported.
 */
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

/** 
 * The qse_mchar_t type defines a multi-byte character type.
 */
typedef char qse_mchar_t;
/**
 * The qse_mcint_t defines a type that can hold a qse_mchar_t value and 
 * #QSE_MCHAR_EOF.
 */
typedef int qse_mcint_t;

/** @typedef qse_wchar_t
 * The qse_wchar_t type defines a wide character type. 
 */
/** @typedef qse_wcint_t
 * The qse_wcint_t type defines a type that can hold a qse_wchar_t value and 
 * #QSE_WCHAR_EOF.
 */
#if defined(__cplusplus) && \
    (!(defined(_MSC_VER) || defined(_SCO_DS)) || \
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

/** @typedef qse_char_t
 * The qse_char_t type defines a character type.
 */
/** @typedef qse_cint_t
 * The qse_cint_t typep defines a type that can hold a qse_char_t value and 
 * #QSE_CHAR_EOF.
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

/**
 * The qse_xstr_t type defines a structure combining a pointer to a character
 * string and the number of characters. It is designed to be interchangeable
 * with the #qse_cstr_t type except the constness on the @a ptr field.
 */
struct qse_xstr_t
{
	qse_char_t* ptr; /**< pointer to a character string */
	qse_size_t  len; /**< the number of characters */
};
typedef struct qse_xstr_t qse_xstr_t;

/**
 * The qse_cstr_t type defines a structure combining a pointer to
 * a constant character string and the number of characters.
 * It is designed to be interchangeable with the #qse_xstr_t type
 * except the constness on the @a ptr field.
 */
struct qse_cstr_t
{
	const qse_char_t* ptr; /**< pointer to a const character string */
	qse_size_t        len; /**< the number of characters */
};
typedef struct qse_cstr_t qse_cstr_t;

/**
 * The qse_mmgr_t type defines a set type of functions for memory management.
 * As the type is merely a structure, it is just used as a single container
 * for memory management functions with a pointer to user-defined data. 
 * The user-defined \a data is passed to each memory management function
 * whenever it is called. You can allocate, reallocate, and free a memory
 * chunk.
 *
 * For example, a qse_xxx_open() function accepts a pointer of the qse_mmgr_t *
 * type and the xxx object uses it to manage dynamic data within the object. 
 */
struct qse_mmgr_t
{
	/** 
	 * allocate a memory chunk of the size \a n.
	 * @return a pointer to a memory chunk on success, QSE_NULL on failure.
	 */
	void* (*alloc)   (void* udd, qse_size_t n);
	/** 
	 * resize a memory chunk pointed to by \a ptr to the size \a n.
	 * @return a pointer to a memory chunk on success, QSE_NULL on failure.
	 */
	void* (*realloc) (void* udd, void* ptr, qse_size_t n);
	/**
	 * frees a memory chunk pointed to by \a ptr.
	 */
	void  (*free)    (void* udd, void* ptr);
	/** 
	 * a pointer to user-defined data passed as the first parameter to
	 * alloc(), realloc(), and free().
	 */
	void*   udd;
};
typedef struct qse_mmgr_t qse_mmgr_t;
/******/

#endif
