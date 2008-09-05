/*
 * $Id: types.h 363 2008-09-04 10:58:08Z baconevi $
 *
 * {License}
 */

#ifndef _ASE_TYPES_H_
#define _ASE_TYPES_H_

/**
 * @file types.h
 * @brief Define common data types
 *
 * This file defines various data types to enhance portability across
 * different platforms. 
 */

#if defined(_WIN32)
	#include <ase/conf_msw.h>
#elif defined(vms) || defined(__vms)
	#include <ase/conf_vms.h>
#elif defined(__unix__) || defined(__unix) || defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) || (defined(__APPLE__) && defined(__MACH__))  || defined(__SPU__)
	#if !defined(__unix__)
		#define __unix__
	#endif
	#if !defined(__unix)
		#define __unix
	#endif
	#include <ase/config.h>
#else
	#error unsupported operating system
#endif

/** 
 * @brief a boolean type 
 * 
 * This type defines a boolean type as represented
 * by #ASE_TRUE and #ASE_FALSE.
 */
typedef int ase_bool_t;
/** Represents a boolean true value */
#define ASE_TRUE  (0 == 0)
/** Represents a boolean false value */
#define ASE_FALSE (0 != 0)


/** 
 * @brief a tri-state type 
 *
 * This type defines a tri-state type as represented 
 * by ASE_ALIVE, ASE_ZOMBIE, and ASE_DEAD.
 */
typedef int ase_tri_t;
#define ASE_ALIVE   1
#define ASE_ZOMBIE  0
#define ASE_DEAD   -1

/** 
 *  @typedef ase_int_t
 *  @brief a signed integer type that can hold a pointer
 */
/**
 *  @typedef ase_uint_t
 *  @brief a unsigned integer type that can hold a pointer 
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


/**
 *  @typedef ase_long_t
 *  @brief the largest signed integer type supported by the system
 */
/**
 *  @typedef ase_ulong_t
 *  @brief the largest unsigned integer type supported by the system
 */

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

/**
 *  @typedef ase_int8_t
 *  @brief an 8-bit signed integer type
 */
/**
 *  @typedef ase_uint8_t
 *  @brief an 8-bit unsigned integer type
 */
#if ASE_SIZEOF_CHAR == 1
	typedef char ase_int8_t;
	typedef unsigned char ase_uint8_t;
#elif ASE_SIZEOF___INT8 == 1
	typedef __int8 ase_int8_t;
	typedef unsigned __int8 ase_uint8_t;
#endif

/**
 *  @typedef ase_int16_t
 *  @brief an 16-bit signed integer type
 */
/**
 *  @typedef ase_uint16_t
 *  @brief an 16-bit unsigned integer type
 */
#if ASE_SIZEOF_SHORT == 2
	typedef short ase_int16_t;
	typedef unsigned short ase_uint16_t;
#elif ASE_SIZEOF___INT16 == 2
	typedef __int16 ase_int16_t;
	typedef unsigned __int16 ase_uint16_t;
#endif

/**
 *  @typedef ase_int32_t
 *  @brief an 32-bit signed integer type
 */
/**
 *  @typedef ase_uint32_t
 *  @brief an 32-bit unsigned integer type
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

/**
 *  @typedef ase_int64_t
 *  @brief an 64-bit signed integer type
 */
/**
 *  @typedef ase_uint64_t
 *  @brief an 64-bit unsigned integer type
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

/** an 8-bit unsigned integer type */
typedef ase_uint8_t ase_byte_t;
/** an unsigned integer type that can hold a pointer value */
typedef ase_uint_t  ase_size_t;
/** an signed integer type that can hold a pointer value */
typedef ase_int_t   ase_ssize_t;
/** an integer type identical to ase_uint_t */
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

#if defined(__cplusplus) && (!defined(_MSC_VER) || (defined(_MSC_VER)&&defined(_NATIVE_WCHAR_T_DEFINED)))
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
	#elif defined(hpux) || defined(__hpux) || defined(__hpux__)
		#if defined(__HP_cc) || defined(__HP_aCC)
		typedef unsigned int ase_wchar_t;
		#else
		typedef int ase_wchar_t;
		#endif
		typedef int ase_wcint_t;
	#elif defined(__linux) && (ASE_SIZEOF_INT == 4)
		typedef int ase_wchar_t;
		typedef int ase_wcint_t;
	#elif defined(__linux) && (ASE_SIZEOF_LONG == 4)
		typedef long ase_wchar_t;
		typedef long ase_wcint_t;
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

typedef void* (*ase_alloc_t)   (void* data, ase_size_t n);
typedef void* (*ase_realloc_t) (void* data, void* ptr, ase_size_t n);
typedef void  (*ase_free_t)    (void* data, void* ptr);

typedef ase_bool_t (*ase_isccls_t) (void* data, ase_cint_t c);
typedef ase_cint_t (*ase_toccls_t) (void* data, ase_cint_t c);

struct ase_cstr_t
{
	const ase_char_t* ptr;
	ase_size_t        len;
};

struct ase_mmgr_t
{
	ase_alloc_t   alloc;
	ase_realloc_t realloc;
	ase_free_t    free;
	void*         data;
};

struct ase_ccls_t
{
	ase_bool_t (*is) (void* data, ase_cint_t c, int type);
	ase_cint_t (*to) (void* data, ase_cint_t c, int type);
	void* data;
};


/* 
 * NAME: determine the size of data
 *
 * DESCRIPTION:
 *  The ase_sizer_t is a generic type used by many other modules usually to
 *  get the new size for resizing data structure.
 */
typedef ase_size_t (*ase_sizer_t) (void* data);

#endif
