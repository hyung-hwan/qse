/*
 * $Id: types.h,v 1.57 2006-10-22 11:44:46 bacon Exp $
 */

#ifndef _SSE_TYPES_H_
#define _SSE_TYPES_H_

#if defined(_WIN32)
	#include <sse/conf_msw.h>
#elif defined(vms) || defined(__vms)
	#include <sse/conf_vms.h>
#elif defined(dos) || defined(__dos)
	#include <sse/conf_dos.h>
#else
	#include <sse/config.h>
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
 * TYPE: sse_bool_t
 *   Boolean type
 *
 * TYPE: sse_tri_t
 *   Tri-State type
 *
 * GROUP: Integral Types
 *
 * TYPE: sse_byte_t
 * 
 * TYPE: sse_word_t
 *
 * TYPE: sse_int_t
 *   Signed integer whose size equals the size of a pointer
 *
 * TYPE: sse_uint_t
 *   Unsigned integer whose size equals the size of a pointer
 * 
 * TYPE: sse_long_t
 *   The largest signed integer available
 *
 * TYPE: sse_ulong_t
 *   The largest unsigned integer available
 * 
 * TYPE: sse_size_t
 *   Unsigned integer which can represent the size of the largest 
 *   in-memory data
 *
 * TYPE: sse_ssize_t
 *   Signed version of sse_size_t
 *
 * TYPE: sse_int8_t
 *
 * TYPE: sse_uint8_t
 *
 * TYPE: sse_int16_t
 *
 * TYPE: sse_uint16_t
 *
 * TYPE: sse_int32_t
 * 
 * TYPE: sse_uint32_t
 *
 * TYPE: sse_int64_t
 *
 * TYPE: sse_uint64_t
 */

/* boolean type */
typedef int sse_bool_t;
#define sse_true  (0 == 0)
#define sse_false (0 != 0)

/* tri-state type */
typedef int sse_tri_t;
#define sse_alive   1
#define sse_zombie  0
#define sse_dead   -1

/* integer that can hold a pointer */
#if SSE_SIZEOF_VOID_P == SSE_SIZEOF_INT
	typedef int sse_int_t;
	typedef unsigned int sse_uint_t;
#elif SSE_SIZEOF_VOID_P == SSE_SIZEOF_LONG
	typedef long sse_int_t;
	typedef unsigned long sse_uint_t;
#elif SSE_SIZEOF_VOID_P == SSE_SIZEOF_LONG_LONG
	typedef long long sse_int_t;
	typedef unsigned long long sse_uint_t;
#else
	#error Unsupported pointer size
#endif


/* the largest integer supported by the system */
#if SSE_SIZEOF_LONG_LONG != 0
	typedef long long sse_long_t;
	typedef unsigned long long sse_ulong_t;
#elif SSE_SIZEOF___INT64 != 0
	typedef __int64 sse_long_t;
	typedef unsigned __int64 sse_ulong_t;
#else
	typedef long sse_long_t;
	typedef unsigned long sse_ulong_t;
#endif

/* integers of specific size */
#if SSE_SIZEOF_CHAR == 1
	typedef char sse_int8_t;
	typedef unsigned char sse_uint8_t;
#elif SSE_SIZEOF___INT8 == 1
	typedef __int8 sse_int8_t;
	typedef unsigned __int8 sse_uint8_t;
#endif

#if SSE_SIZEOF_SHORT == 2
	typedef short sse_int16_t;
	typedef unsigned short sse_uint16_t;
#elif SSE_SIZEOF___INT16 == 2
	typedef __int16 sse_int16_t;
	typedef unsigned __int16 sse_uint16_t;
#endif

#if SSE_SIZEOF_INT == 4
	typedef int sse_int32_t;
	typedef unsigned int sse_uint32_t;
#elif SSE_SIZEOF_LONG == 4
	typedef long sse_int32_t;
	typedef unsigned long sse_uint32_t;
#elif SSE_SIZEOF___INT32 == 4
	typedef __int32 sse_int32_t;
	typedef unsigned __int32 sse_uint32_t;
#endif

#if SSE_SIZEOF_INT == 8
	#define SSE_HAVE_INT64_T
	#define SSE_HAVE_UINT64_T
	typedef int sse_int64_t;
	typedef unsigned int sse_uint64_t;
#elif SSE_SIZEOF_LONG == 8
	#define SSE_HAVE_INT64_T
	#define SSE_HAVE_UINT64_T
	typedef long sse_int64_t;
	typedef unsigned long sse_uint64_t;
#elif SSE_SIZEOF_LONG_LONG == 8
	#define SSE_HAVE_INT64_T
	#define SSE_HAVE_UINT64_T
	typedef long long sse_int64_t;
	typedef unsigned long long sse_uint64_t;
#elif SSE_SIZEOF___INT64 == 8
	#define SSE_HAVE_INT64_T
	#define SSE_HAVE_UINT64_T
	typedef __int64 sse_int64_t;
	typedef unsigned __int64 sse_uint64_t;
#endif

#if SSE_SIZEOF_INT == 16
	#define SSE_HAVE_INT128_T
	#define SSE_HAVE_UINT128_T
	typedef int sse_int128_t;
	typedef unsigned int sse_uint128_t;
#elif SSE_SIZEOF_LONG == 16
	#define SSE_HAVE_INT128_T
	#define SSE_HAVE_UINT128_T
	typedef long sse_int128_t;
	typedef unsigned long sse_uint128_t;
#elif SSE_SIZEOF_LONG_LONG == 16
	#define SSE_HAVE_INT128_T
	#define SSE_HAVE_UINT128_T
	typedef long long sse_int128_t;
	typedef unsigned long long sse_uint128_t;
#elif SSE_SIZEOF___INT128 == 16
	#define SSE_HAVE_INT128_T
	#define SSE_HAVE_UINT128_T
	typedef __int128 sse_int128_t;
	typedef unsigned __int128 sse_uint128_t;
#endif

/* miscellaneous integral types */
typedef sse_uint8_t sse_byte_t;
typedef sse_uint_t  sse_size_t;
typedef sse_int_t   sse_ssize_t;
typedef sse_uint_t  sse_word_t;

/* floating-point number */
#if defined(__FreeBSD__)
	/* TODO: check if the support for long double is complete.
	 *       if so, use long double for sse_real_t */
	#define SSE_SIZEOF_REAL SSE_SIZEOF_DOUBLE
	typedef double sse_real_t;
#elif SSE_SIZEOF_LONG_DOUBLE > SSE_SIZEOF_DOUBLE
	#define SSE_SIZEOF_REAL SSE_SIZEOF_LONG_DOUBLE
	typedef long double sse_real_t;
#else
	#define SSE_SIZEOF_REAL SSE_SIZEOF_DOUBLE
	typedef double sse_real_t;
#endif

/* character types */
typedef char sse_mchar_t;
typedef int  sse_mcint_t;

#if defined(__cplusplus)
	/* C++ */
	typedef wchar_t sse_wchar_t;
	typedef wchar_t sse_wcint_t;

	/* all the way down from here for C */
#elif (SSE_SIZEOF_WCHAR_T == 2) || (SSE_SIZEOF_WCHAR_T == 0)
	typedef unsigned short sse_wchar_t;
	typedef unsigned short sse_wcint_t;
#elif (SSE_SIZEOF_WCHAR_T == 4)
	#if defined(vms) || defined(__vms)
		typedef unsigned int sse_wchar_t;
		typedef int sse_wcint_t;
	#elif defined(__FreeBSD__)
		typedef int sse_wchar_t;
		typedef int sse_wcint_t;
	#elif (defined(sun) || defined(__sun)) && defined(_LP64)
		typedef int sse_wchar_t;
		typedef int sse_wcint_t;
	#elif SSE_SIZEOF_LONG == 4
		typedef long sse_wchar_t;
		typedef long sse_wcint_t;
	#else
		typedef int sse_wchar_t;
		typedef int sse_wcint_t;
	#endif
#else
	#error unsupported size of wchar_t
#endif

#if defined(_WIN32) && (defined(UNICODE)||defined(_UNICODE))
	#define SSE_CHAR_IS_WCHAR
	typedef sse_wchar_t sse_char_t;
	typedef sse_wcint_t sse_cint_t;
#else
	#if defined(SSE_CHAR_IS_MCHAR)
		typedef sse_mchar_t sse_char_t;
		typedef sse_mcint_t sse_cint_t;
	#elif defined(SSE_CHAR_IS_WCHAR)
		typedef sse_wchar_t sse_char_t;
		typedef sse_wcint_t sse_cint_t;
	#elif defined(_MBCS)
		#define SSE_CHAR_IS_MCHAR
		typedef sse_mchar_t sse_char_t;
		typedef sse_mcint_t sse_cint_t;
	#else
		#define SSE_CHAR_IS_WCHAR
		typedef sse_wchar_t sse_char_t;
		typedef sse_wcint_t sse_cint_t;
	#endif
#endif

#if defined(SSE_CHAR_IS_WCHAR) && defined(_WIN32) 
	#ifndef UNICODE
		#define UNICODE
	#endif
	#ifndef _UNICODE
		#define _UNICODE
	#endif
#endif

#endif
