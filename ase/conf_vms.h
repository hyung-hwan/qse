/*
 * $Id: conf_vms.h,v 1.8 2007-02-03 10:52:36 bacon Exp $
 *
 * {License}
 */

/* TODO: please complete the itanium portion */

/* both vax and alpha are in the little endian. */
#define ASE_ENDIAN_LITTLE 

#define ASE_SIZEOF_CHAR 1
#define ASE_SIZEOF_SHORT 2
#define ASE_SIZEOF_INT 4
#define ASE_SIZEOF_LONG 4

#if defined(vax) || defined(__vax)
	#define ASE_SIZEOF_LONG_LONG 0
#elif defined(alpha) || defined(__alpha)
	#define ASE_SIZEOF_LONG_LONG 8
#elif defined(itanium) || defined(__itanium)
	#define ASE_SIZEOF_LONG_LONG 8
#else
	#define ASE_SIZEOF_LONG_LONG 0
#endif

#define ASE_SIZEOF___INT8 1
#define ASE_SIZEOF___INT16 2
#define ASE_SIZEOF___INT32 4

#if defined(vax) || defined(__vax)
	#define ASE_SIZEOF___INT64 0
#elif defined(alpha) || defined(__alpha)
	#define ASE_SIZEOF___INT64 8
#elif defined(itanium) || defined(__itanium)
	#define ASE_SIZEOF___INT64 8
#else
	#define ASE_SIZEOF___INT64 0
#endif

#define ASE_SIZEOF___INT96 0
#define ASE_SIZEOF___INT128 0

#if defined(vax) || defined(__vax)
	#define ASE_SIZEOF_VOID_P 4
#elif defined(alpha) || defined(__alpha)
	/*#pragma pointer_size 32
	#define ASE_SIZEOF_VOID_P 4*/
	#pragma pointer_size 64
	#define ASE_SIZEOF_VOID_P 8
#elif defined(itanium) || defined(__itanium)
	/*#pragma pointer_size 32
	#define ASE_SIZEOF_VOID_P 4*/
	#pragma pointer_size 64
	#define ASE_SIZEOF_VOID_P 8
#else
	#define ASE_SIZEOF_VOID_P 0
#endif

#define ASE_SIZEOF_FLOAT 4
#define ASE_SIZEOF_DOUBLE 8

#if defined(vax) || defined(__vax)
	#define ASE_SIZEOF_LONG_DOUBLE 8
#elif defined(alpha) || defined(__alpha)
	#define ASE_SIZEOF_LONG_DOUBLE 16
#elif defined(itanium) || defined(__itanium)
	#define ASE_SIZEOF_LONG_DOUBLE 16
#else
	#define ASE_SIZEOF_LONG_DOUBLE 0
#endif

#define ASE_SIZEOF_WCHAR_T 4
