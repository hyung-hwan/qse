/*
 * $Id: types.h,v 1.55 2006-09-08 11:27:01 bacon Exp $
 */

#ifndef _XP_TYPES_H_
#define _XP_TYPES_H_

#if defined(_WIN32)
	#include <xp/conf_msw.h>
#elif defined(vms) || defined(__vms)
	#include <xp/conf_vms.h>
#elif defined(dos) || defined(__dos)
	#include <xp/conf_dos.h>
#else
	#include <xp/config.h>
#endif

/*
 * NOTE: Data Types
 *   Various basic data types are predefined for convenience sake and used
 *   constantly thoughout the entire toolkit. The developers are strongly
 *   encouraged to use these data types rather than built-in data types
 *   supported by compilers. 
 *
 * NOTE: Availabilty 
 *   Some of the data types may or may not be available depending on
 *   the capability of the compiler.
 *
 * GROUP: State Types
 *
 * TYPE: xp_bool_t
 *   Boolean type
 *
 * TYPE: xp_tri_t
 *   Tri-State type
 *
 * GROUP: Integral Types
 *
 * TYPE: xp_byte_t
 * 
 * TYPE: xp_word_t
 *
 * TYPE: xp_int_t
 *   Signed integer whose size equals the size of a pointer
 *
 * TYPE: xp_uint_t
 *   Unsigned integer whose size equals the size of a pointer
 * 
 * TYPE: xp_long_t
 *   The largest signed integer available
 *
 * TYPE: xp_ulong_t
 *   The largest unsigned integer available
 * 
 * TYPE: xp_size_t
 *   Unsigned integer which can represent the size of the largest 
 *   in-memory data
 *
 * TYPE: xp_ssize_t
 *   Signed version of xp_size_t
 *
 * TYPE: xp_int8_t
 *
 * TYPE: xp_uint8_t
 *
 * TYPE: xp_int16_t
 *
 * TYPE: xp_uint16_t
 *
 * TYPE: xp_int32_t
 * 
 * TYPE: xp_uint32_t
 *
 * TYPE: xp_int64_t
 *
 * TYPE: xp_uint64_t
 */

/* boolean type */
typedef int xp_bool_t;
#define xp_true  (0 == 0)
#define xp_false (0 != 0)

/* tri-state type */
typedef int xp_tri_t;
#define xp_alive   1
#define xp_zombie  0
#define xp_dead   -1

/* integer that can hold a pointer */
#if XP_SIZEOF_VOID_P == XP_SIZEOF_LONG_LONG
	typedef long long xp_int_t;
	typedef unsigned long long xp_uint_t;
#elif XP_SIZEOF_VOID_P == XP_SIZEOF_LONG
	typedef long xp_int_t;
	typedef unsigned long xp_uint_t;
#else
	typedef int xp_int_t;
	typedef unsigned int xp_uint_t;
#endif

/* the largest integer supported by the system */
#if XP_SIZEOF_LONG_LONG != 0
	typedef long long xp_long_t;
	typedef unsigned long long xp_ulong_t;
#elif XP_SIZEOF___INT64 != 0
	typedef __int64 xp_long_t;
	typedef unsigned __int64 xp_ulong_t;
#else
	typedef long xp_long_t;
	typedef unsigned long xp_ulong_t;
#endif

/* integers of specific size */
#if XP_SIZEOF_CHAR == 1
	typedef char xp_int8_t;
	typedef unsigned char xp_uint8_t;
#elif XP_SIZEOF___INT8 == 1
	typedef __int8 xp_int8_t;
	typedef unsigned __int8 xp_uint8_t;
#endif

#if XP_SIZEOF_SHORT == 2
	typedef short xp_int16_t;
	typedef unsigned short xp_uint16_t;
#elif XP_SIZEOF___INT16 == 2
	typedef __int16 xp_int16_t;
	typedef unsigned __int16 xp_uint16_t;
#endif

#if XP_SIZEOF_INT == 4
	typedef int xp_int32_t;
	typedef unsigned int xp_uint32_t;
#elif XP_SIZEOF_LONG == 4
	typedef long xp_int32_t;
	typedef unsigned long xp_uint32_t;
#elif XP_SIZEOF___INT32 == 4
	typedef __int32 xp_int32_t;
	typedef unsigned __int32 xp_uint32_t;
#endif

#if XP_SIZEOF_INT == 8
	#define XP_HAVE_INT64_T
	#define XP_HAVE_UINT64_T
	typedef int xp_int64_t;
	typedef unsigned int xp_uint64_t;
#elif XP_SIZEOF_LONG == 8
	#define XP_HAVE_INT64_T
	#define XP_HAVE_UINT64_T
	typedef long xp_int64_t;
	typedef unsigned long xp_uint64_t;
#elif XP_SIZEOF_LONG_LONG == 8
	#define XP_HAVE_INT64_T
	#define XP_HAVE_UINT64_T
	typedef long long xp_int64_t;
	typedef unsigned long long xp_uint64_t;
#elif XP_SIZEOF___INT64 == 8
	#define XP_HAVE_INT64_T
	#define XP_HAVE_UINT64_T
	typedef __int64 xp_int64_t;
	typedef unsigned __int64 xp_uint64_t;
#endif

#if XP_SIZEOF_INT == 16
	#define XP_HAVE_INT128_T
	#define XP_HAVE_UINT128_T
	typedef int xp_int128_t;
	typedef unsigned int xp_uint128_t;
#elif XP_SIZEOF_LONG == 16
	#define XP_HAVE_INT128_T
	#define XP_HAVE_UINT128_T
	typedef long xp_int128_t;
	typedef unsigned long xp_uint128_t;
#elif XP_SIZEOF_LONG_LONG == 16
	#define XP_HAVE_INT128_T
	#define XP_HAVE_UINT128_T
	typedef long long xp_int128_t;
	typedef unsigned long long xp_uint128_t;
#elif XP_SIZEOF___INT128 == 16
	#define XP_HAVE_INT128_T
	#define XP_HAVE_UINT128_T
	typedef __int128 xp_int128_t;
	typedef unsigned __int128 xp_uint128_t;
#endif

/* miscellaneous integral types */
typedef xp_uint8_t xp_byte_t;
typedef xp_uint_t  xp_size_t;
typedef xp_int_t   xp_ssize_t;
typedef xp_uint_t  xp_word_t;

/* floating-point number */
#if defined(__FreeBSD__)
	/* TODO: check if the support for long double is complete.
	 *       if so, use long double for xp_real_t */
	#define XP_SIZEOF_REAL XP_SIZEOF_DOUBLE
	typedef double xp_real_t;
#elif XP_SIZEOF_LONG_DOUBLE > XP_SIZEOF_DOUBLE
	#define XP_SIZEOF_REAL XP_SIZEOF_LONG_DOUBLE
	typedef long double xp_real_t;
#else
	#define XP_SIZEOF_REAL XP_SIZEOF_DOUBLE
	typedef double xp_real_t;
#endif

/* character types */
typedef char xp_mchar_t;
typedef int  xp_mcint_t;

#if defined(__cplusplus)
	/* C++ */
	typedef wchar_t xp_wchar_t;
	typedef wchar_t xp_wcint_t;

	/* all the way down from here for C */
#elif (XP_SIZEOF_WCHAR_T == 2) || (XP_SIZEOF_WCHAR_T == 0)
	typedef unsigned short xp_wchar_t;
	typedef unsigned short xp_wcint_t;
#elif (XP_SIZEOF_WCHAR_T == 4)
	#if defined(vms) || defined(__vms)
		typedef unsigned int xp_wchar_t;
		typedef int xp_wcint_t;
	#elif defined(__FreeBSD__)
		typedef int xp_wchar_t;
		typedef int xp_wcint_t;
	#elif (defined(sun) || defined(__sun)) && defined(_LP64)
		typedef int xp_wchar_t;
		typedef int xp_wcint_t;
	#elif XP_SIZEOF_LONG == 4
		typedef long xp_wchar_t;
		typedef long xp_wcint_t;
	#else
		typedef int xp_wchar_t;
		typedef int xp_wcint_t;
	#endif
#else
	#error unsupported size of wchar_t
#endif

#if defined(_WIN32) && (defined(UNICODE)||defined(_UNICODE))
	#define XP_CHAR_IS_WCHAR
	typedef xp_wchar_t xp_char_t;
	typedef xp_wcint_t xp_cint_t;
#else
	#if defined(XP_CHAR_IS_MCHAR)
		typedef xp_mchar_t xp_char_t;
		typedef xp_mcint_t xp_cint_t;
	#elif defined(XP_CHAR_IS_WCHAR)
		typedef xp_wchar_t xp_char_t;
		typedef xp_wcint_t xp_cint_t;
	#elif defined(_MBCS)
		#define XP_CHAR_IS_MCHAR
		typedef xp_mchar_t xp_char_t;
		typedef xp_mcint_t xp_cint_t;
	#else
		#define XP_CHAR_IS_WCHAR
		typedef xp_wchar_t xp_char_t;
		typedef xp_wcint_t xp_cint_t;
	#endif
#endif

#if defined(XP_CHAR_IS_WCHAR) && defined(_WIN32) 
	#ifndef UNICODE
		#define UNICODE
	#endif
	#ifndef _UNICODE
		#define _UNICODE
	#endif
#endif

#endif
