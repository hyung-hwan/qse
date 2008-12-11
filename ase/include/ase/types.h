/*
 * $Id: types.h 468 2008-12-10 10:19:59Z baconevi $
 *
 * {License}
 */

#ifndef _ASE_TYPES_H_
#define _ASE_TYPES_H_

/****o* ase/basic types
 * DESCRIPTION
 *  <ase/types.h> defines various common basic types. They are designed to be
 *  cross-platform. These types are preferred over native data types in many
 *  contexts.
 *
 *  #include <ase/types.h>
 ******
 */

#if defined(HAVE_CONFIG_H)
	#include <ase/config.h>
#elif defined(_WIN32)
	#include <ase/conf_msw.h>
#elif defined(vms) || defined(__vms)
	#include <ase/conf_vms.h>
/*
#elif defined(__unix__) || defined(__unix) || defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) || (defined(__APPLE__) && defined(__MACH__))  || defined(__SPU__)
	#if !defined(__unix__)
		#define __unix__
	#endif
	#if !defined(__unix)
		#define __unix
	#endif
	#include <ase/config.h>
*/
#else
	#error unsupported operating system
#endif

/****t* ase/ase_bool_t
 * NAME
 *  ase_bool_t - define a boolean type
 * DESCRIPTION
 *  The ase_bool_t type defines a boolean type that can represent ASE_TRUE 
 *  and ASE_FALSE.
 ******
 */
typedef int ase_bool_t;

/****t* ase/ase_tri_t
 * NAME
 *  ase_tri_t - define a tri-state type
 * DESCRIPTION
 *  The ase_tri_t type defines a tri-state type that can represent ASE_ALIVE,
 *  ASE_ZOMBIE, and ASE_DEAD.
 ******
 */
typedef int ase_tri_t;

/****t* ase/ase_int_t,ase_uint_t
 * NAME
 *  ase_int_t - define a signed integer type as large as a pointer type
 *  ase_uint_t - define an unsigned integer type as large as a pointer type
 ******
 */
#if (defined(hpux) || defined(__hpux) || defined(__hpux__)) && (ASE_SIZEOF_VOID_P == ASE_SIZEOF_LONG)
	typedef long ase_int_t;
	typedef unsigned long ase_uint_t;
#elif defined(__SPU__) && (ASE_SIZEOF_VOID_P == ASE_SIZEOF_LONG)
	typedef long ase_int_t;
	typedef unsigned long ase_uint_t;
#elif ASE_SIZEOF_VOID_P == ASE_SIZEOF_INT
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

/****t* ase/ase_long_t,ase_ulong_t
 * NAME
 *  ase_long_t - define the largest signed integer type supported
 *  ase_ulong_t - define the largest unsigned integer type supported
 ******
 */
#if ASE_SIZEOF_LONG_LONG > 0
	typedef long long ase_long_t;
	typedef unsigned long long ase_ulong_t;
#elif ASE_SIZEOF___INT64 > 0
	typedef __int64 ase_long_t;
	typedef unsigned __int64 ase_ulong_t;
#else
	typedef long ase_long_t;
	typedef unsigned long ase_ulong_t;
#endif

/****t* ase/ase_int8_t,ase_uint8_t
 * NAME
 *  ase_int8_t - define an 8-bit signed integer type
 *  ase_uint8_t - define an 8-bit unsigned integer type
 ******
 */
#if ASE_SIZEOF_CHAR == 1
	typedef char ase_int8_t;
	typedef unsigned char ase_uint8_t;
#elif ASE_SIZEOF___INT8 == 1
	typedef __int8 ase_int8_t;
	typedef unsigned __int8 ase_uint8_t;
#endif

/****t* ase/ase_int16_t,ase_uint16_t
 * NAME
 *  ase_int16_t - define a 16-bit signed integer type
 *  ase_uint16_t - define a 16-bit unsigned integer type
 ******
 */
#if ASE_SIZEOF_SHORT == 2
	typedef short ase_int16_t;
	typedef unsigned short ase_uint16_t;
#elif ASE_SIZEOF___INT16 == 2
	typedef __int16 ase_int16_t;
	typedef unsigned __int16 ase_uint16_t;
#endif

/****t* ase/ase_int32_t,ase_uint32_t
 * NAME
 *  ase_int32_t - define a 32-bit signed integer type
 *  ase_uint32_t - define a 32-bit unsigned integer type
 ******
 */
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

/****t* ase/ase_int64_t,ase_uint64_t
 * NAME
 *  ase_int64_t - define a 64-bit signed integer type
 *  ase_uint64_t - define a 64-bit unsigned integer type
 ******
 */
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

/****t* ase/ase_byte_t
 * NAME
 *  ase_word_t - define a byte type
 ******
 */
typedef ase_uint8_t ase_byte_t;

/****t* ase/ase_size_t
 * NAME
 *  ase_size_t - define an unsigned integer type that can hold a pointer value
 ******
 */
#ifdef __SIZE_TYPE__
typedef __SIZE_TYPE__ ase_size_t;
#else
typedef ase_uint_t  ase_size_t;
#endif

/****t* ase/ase_ssize_t
 * NAME
 *  ase_ssize_t - define an signed integer type that can hold a pointer value
 ******
 */
typedef ase_int_t   ase_ssize_t;

/****t* ase/ase_word_t
 * NAME
 *  ase_word_t - define an integer type identical to ase_uint_t 
 ******
 */
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

/****t* ase/ase_mchar_t,ase_mcint_t
 * NAME
 *  ase_mchar_t - define a multi-byte character
 *  ase_mcint_t - define a type that can hold ase_mchar_t and ASE_MCHAR_EOF
 ******
 */
typedef char ase_mchar_t;
typedef int  ase_mcint_t;

/****t* ase/ase_wchar_t,ase_wcint_t
 * NAME
 *  ase_wchar_t - define a wide character
 *  ase_wcint_t - define a type that can hold ase_wchar_t and ASE_WCHAR_EOF
 ******
 */
#if defined(__cplusplus) && (!defined(_MSC_VER) || (defined(_MSC_VER)&&defined(_NATIVE_WCHAR_T_DEFINED)))
	/* C++ */

	typedef wchar_t ase_wchar_t;
	typedef wchar_t ase_wcint_t;

	/* all the way down from here for C */
#elif defined(__WCHAR_TYPE__) && defined(__WINT_TYPE__)
	typedef __WCHAR_TYPE__ ase_wchar_t;
	typedef __WINT_TYPE__ ase_wcint_t;
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
	#elif (defined(sun) || defined(__sun) || defined(__linux))
		#if defined(_LP64)
			typedef int ase_wchar_t;
			typedef int ase_wcint_t;
		#else
			typedef long ase_wchar_t;
			typedef long ase_wcint_t;
		#endif
	#elif defined(__APPLE__) && defined(__MACH__)
		typedef int ase_wchar_t;
		typedef int ase_wcint_t;
	#elif defined(hpux) || defined(__hpux) || defined(__hpux__)
		#if defined(__HP_cc) || defined(__HP_aCC)
		typedef unsigned int ase_wchar_t;
		#else
		typedef int ase_wchar_t;
		#endif
		typedef int ase_wcint_t;
	#elif ASE_SIZEOF_LONG == 4
		typedef long ase_wchar_t;
		typedef long ase_wcint_t;
	#elif ASE_SIZEOF_INT == 4
		typedef int ase_wchar_t;
		typedef int ase_wcint_t;
	#else
		#error no supported data type for wchar_t
	#endif
#else
	#error unsupported size of wchar_t
#endif

/****t* ase/ase_char_t,ase_cint_t
 * NAME
 *  ase_char_t - define a character
 *  ase_cint_t - define a type that can hold ase_char_t and ASE_CHAR_EOF
 ******
 */
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

typedef struct ase_xstr_t ase_xstr_t;
typedef struct ase_cstr_t ase_cstr_t;
typedef struct ase_mmgr_t ase_mmgr_t;
typedef struct ase_ccls_t ase_ccls_t;

/****t* ase/ase_xstr_t
 * NAME
 *  ase_xstr_t - combine a pointer and length 
 *
 * SYNOPSIS
 */
struct ase_xstr_t
{
	ase_char_t* ptr; /* this is not a const pointer */
	ase_size_t  len;
};
/******/

/****t* ase/ase_cstr_t
 * NAME
 *  ase_cstr_t - combine a constant pointer and length 
 *
 * SYNOPSIS
 */
struct ase_cstr_t
{
	const ase_char_t* ptr; /* this is a const pointer */
	ase_size_t        len;
};
/******/

/****t* ase/ase_mmgr_t
 * NAME
 *  ase_mmgr_t - define a memory manager
 *
 * SYNOPSIS
 */
struct ase_mmgr_t
{
	void* (*alloc)   (void* data, ase_size_t n);
	void* (*realloc) (void* data, void* ptr, ase_size_t n);
	void  (*free)    (void* data, void* ptr);
	void*   data;
};
/******/

/****t* ase/ase_ccls_type_t
 * NAME
 *  ase_ccls_type_t - define types of character class
 *
 * SYNOPSIS
 */
enum ase_ccls_type_t
{
        ASE_CCLS_UPPER,
        ASE_CCLS_LOWER,
        ASE_CCLS_ALPHA,
        ASE_CCLS_DIGIT,
        ASE_CCLS_XDIGIT,
        ASE_CCLS_ALNUM,
        ASE_CCLS_SPACE,
        ASE_CCLS_PRINT,
        ASE_CCLS_GRAPH,
        ASE_CCLS_CNTRL,
        ASE_CCLS_PUNCT
};
/******/

typedef enum ase_ccls_type_t ase_ccls_type_t;

/****t* ase/ase_ccls_t
 * NAME
 *  ase_mmgr_t - define a character class handler
 *
 * SYNOPSIS
 */
struct ase_ccls_t
{
	ase_bool_t (*is) (void* data, ase_cint_t c, ase_ccls_type_t type);
	ase_cint_t (*to) (void* data, ase_cint_t c, ase_ccls_type_t type);
	void*        data;
};
/******/

#endif
