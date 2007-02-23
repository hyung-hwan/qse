/*
 * $Id: types.h,v 1.72 2007-02-23 08:17:48 bacon Exp $
 *
 * {License}
 */

#ifndef _ASE_TYPES_H_
#define _ASE_TYPES_H_

#if defined(_WIN32)
	#include <ase/conf_msw.h>
#elif defined(vms) || defined(__vms)
	#include <ase/conf_vms.h>
#elif defined(__unix__) || defined(__unix) || defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) || (defined(__APPLE__) && defined(__MACH__))
	#if !defined(__unix__)
		#define __unix__
	#endif
	#if !defined(__unix)
		#define __unix
	#endif
	#include <ase/conf_unx.h>
#else
	#error unsupported operating system
#endif

/* boolean type */
typedef int ase_bool_t;
#define ase_true  (0 == 0)
#define ase_false (0 != 0)

/* tri-state type */
typedef int ase_tri_t;
#define ase_alive   1
#define ase_zombie  0
#define ase_dead   -1

/* integer that can hold a pointer */
#if ASE_SIZEOF_VOID_P == ASE_SIZEOF_INT
	typedef int ase_int_t;
	typedef unsigned int ase_uint_t;
#elif ASE_SIZEOF_VOID_P == ASE_SIZEOF_LONG
	typedef long ase_int_t;
	typedef unsigned long ase_uint_t;
#elif ASE_SIZEOF_VOID_P == ASE_SIZEOF_LONG_LONG
	typedef long long ase_int_t;
	typedef unsigned long long ase_uint_t;
#elif ASE_SIZEOF_VOID_P == ASE_SIZEOF___INT32
	typedef __int32 ase_int_t;
	typedef unsigned __int32 ase_uint_t;
#elif ASE_SIZEOF_VOID_P == ASE_SIZEOF___INT64
	typedef __int64 ase_int_t;
	typedef unsigned __int64 ase_uint_t;
#else
	#error unsupported pointer size
#endif


/* the largest integer supported by the system */
#if ASE_SIZEOF_LONG_LONG != 0
	typedef long long ase_long_t;
	typedef unsigned long long ase_ulong_t;
#elif ASE_SIZEOF___INT64 != 0
	typedef __int64 ase_long_t;
	typedef unsigned __int64 ase_ulong_t;
#else
	typedef long ase_long_t;
	typedef unsigned long ase_ulong_t;
#endif

/* integers of specific size */
#if ASE_SIZEOF_CHAR == 1
	typedef char ase_int8_t;
	typedef unsigned char ase_uint8_t;
#elif ASE_SIZEOF___INT8 == 1
	typedef __int8 ase_int8_t;
	typedef unsigned __int8 ase_uint8_t;
#endif

#if ASE_SIZEOF_SHORT == 2
	typedef short ase_int16_t;
	typedef unsigned short ase_uint16_t;
#elif ASE_SIZEOF___INT16 == 2
	typedef __int16 ase_int16_t;
	typedef unsigned __int16 ase_uint16_t;
#endif

#if ASE_SIZEOF_INT == 4
	typedef int ase_int32_t;
	typedef unsigned int ase_uint32_t;
#elif ASE_SIZEOF_LONG == 4
	typedef long ase_int32_t;
	typedef unsigned long ase_uint32_t;
#elif ASE_SIZEOF___INT32 == 4
	typedef __int32 ase_int32_t;
	typedef unsigned __int32 ase_uint32_t;
#endif

#if ASE_SIZEOF_INT == 8
	#define ASE_HAVE_INT64_T
	#define ASE_HAVE_UINT64_T
	typedef int ase_int64_t;
	typedef unsigned int ase_uint64_t;
#elif ASE_SIZEOF_LONG == 8
	#define ASE_HAVE_INT64_T
	#define ASE_HAVE_UINT64_T
	typedef long ase_int64_t;
	typedef unsigned long ase_uint64_t;
#elif ASE_SIZEOF_LONG_LONG == 8
	#define ASE_HAVE_INT64_T
	#define ASE_HAVE_UINT64_T
	typedef long long ase_int64_t;
	typedef unsigned long long ase_uint64_t;
#elif ASE_SIZEOF___INT64 == 8
	#define ASE_HAVE_INT64_T
	#define ASE_HAVE_UINT64_T
	typedef __int64 ase_int64_t;
	typedef unsigned __int64 ase_uint64_t;
#endif

#if ASE_SIZEOF_INT == 16
	#define ASE_HAVE_INT128_T
	#define ASE_HAVE_UINT128_T
	typedef int ase_int128_t;
	typedef unsigned int ase_uint128_t;
#elif ASE_SIZEOF_LONG == 16
	#define ASE_HAVE_INT128_T
	#define ASE_HAVE_UINT128_T
	typedef long ase_int128_t;
	typedef unsigned long ase_uint128_t;
#elif ASE_SIZEOF_LONG_LONG == 16
	#define ASE_HAVE_INT128_T
	#define ASE_HAVE_UINT128_T
	typedef long long ase_int128_t;
	typedef unsigned long long ase_uint128_t;
#elif ASE_SIZEOF___INT128 == 16
	#define ASE_HAVE_INT128_T
	#define ASE_HAVE_UINT128_T
	typedef __int128 ase_int128_t;
	typedef unsigned __int128 ase_uint128_t;
#endif

/* miscellaneous integral types */
typedef ase_uint8_t ase_byte_t;
typedef ase_uint_t  ase_size_t;
typedef ase_int_t   ase_ssize_t;
typedef ase_uint_t  ase_word_t;

/* floating-point number */
#if defined(__FreeBSD__)
	/* TODO: check if the support for long double is complete.
	 *       if so, use long double for ase_real_t */
	#define ASE_SIZEOF_REAL ASE_SIZEOF_DOUBLE
	typedef double ase_real_t;
#elif ASE_SIZEOF_LONG_DOUBLE > ASE_SIZEOF_DOUBLE
	#define ASE_SIZEOF_REAL ASE_SIZEOF_LONG_DOUBLE
	typedef long double ase_real_t;
#else
	#define ASE_SIZEOF_REAL ASE_SIZEOF_DOUBLE
	typedef double ase_real_t;
#endif

/* character types */
typedef char ase_mchar_t;
typedef int  ase_mcint_t;

#if defined(__cplusplus)
	/* C++ */
	typedef wchar_t ase_wchar_t;
	typedef wchar_t ase_wcint_t;

	/* all the way down from here for C */
#elif (ASE_SIZEOF_WCHAR_T == 2) || (ASE_SIZEOF_WCHAR_T == 0)
	typedef unsigned short ase_wchar_t;
	typedef unsigned short ase_wcint_t;
#elif (ASE_SIZEOF_WCHAR_T == 4)
	#if defined(vms) || defined(__vms)
		typedef unsigned int ase_wchar_t;
		typedef int ase_wcint_t;
	#elif defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)
		typedef int ase_wchar_t;
		typedef int ase_wcint_t;
	#elif (defined(sun) || defined(__sun)) && defined(_LP64)
		typedef int ase_wchar_t;
		typedef int ase_wcint_t;
	#elif defined(__APPLE__) && defined(__MACH__)
		typedef int ase_wchar_t;
		typedef int ase_wcint_t;
	#elif defined(hpux) || defined(__hpux)
		typedef unsigned int ase_wchar_t;
		typedef int ase_wcint_t;
	#elif ASE_SIZEOF_LONG == 4
		typedef long ase_wchar_t;
		typedef long ase_wcint_t;
	#else
		typedef int ase_wchar_t;
		typedef int ase_wcint_t;
	#endif
#else
	#error unsupported size of wchar_t
#endif

#if defined(_WIN32) && (defined(UNICODE)||defined(_UNICODE))
	#define ASE_CHAR_IS_WCHAR
	typedef ase_wchar_t ase_char_t;
	typedef ase_wcint_t ase_cint_t;
#else
	#if defined(ASE_CHAR_IS_MCHAR)
		typedef ase_mchar_t ase_char_t;
		typedef ase_mcint_t ase_cint_t;
	#elif defined(ASE_CHAR_IS_WCHAR)
		typedef ase_wchar_t ase_char_t;
		typedef ase_wcint_t ase_cint_t;
	#elif defined(_MBCS)
		#define ASE_CHAR_IS_MCHAR
		typedef ase_mchar_t ase_char_t;
		typedef ase_mcint_t ase_cint_t;
	#else
		#define ASE_CHAR_IS_WCHAR
		typedef ase_wchar_t ase_char_t;
		typedef ase_wcint_t ase_cint_t;
	#endif
#endif

#if defined(ASE_CHAR_IS_WCHAR) && defined(_WIN32) 
	#ifndef UNICODE
		#define UNICODE
	#endif
	#ifndef _UNICODE
		#define _UNICODE
	#endif
#endif

typedef struct ase_cstr_t ase_cstr_t;
typedef struct ase_mmgr_t ase_mmgr_t;
typedef struct ase_ccls_t ase_ccls_t;

typedef void* (*ase_malloc_t)  (ase_mmgr_t* mmgr, ase_size_t n);
typedef void* (*ase_realloc_t) (ase_mmgr_t* mmgr, void* ptr, ase_size_t n);
typedef void  (*ase_free_t)    (ase_mmgr_t* mmgr, void* ptr);

typedef ase_bool_t (*ase_isccls_t) (ase_ccls_t* ccls, ase_cint_t c);
typedef ase_cint_t (*ase_toccls_t) (ase_ccls_t* ccls, ase_cint_t c);

struct ase_cstr_t
{
	ase_char_t* ptr;
	ase_size_t  len;
};

struct ase_mmgr_t
{
	ase_malloc_t  malloc;
	ase_realloc_t realloc;
	ase_free_t    free;
	void*         custom_data;
};

struct ase_ccls_t
{
	ase_isccls_t is_upper;
	ase_isccls_t is_lower;
	ase_isccls_t is_alpha;
	ase_isccls_t is_digit;
	ase_isccls_t is_xdigit;
	ase_isccls_t is_alnum;
	ase_isccls_t is_space;
	ase_isccls_t is_print;
	ase_isccls_t is_graph;
	ase_isccls_t is_cntrl;
	ase_isccls_t is_punct;
	ase_toccls_t to_upper;
	ase_toccls_t to_lower;
	void*        custom_data;
};

#endif
