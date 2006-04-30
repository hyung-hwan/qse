/*
 * $Id: conf_vms.h,v 1.4 2006-04-30 17:35:37 bacon Exp $
 */

/* TODO: please complete the itanium portion */

/* both vax and alpha are in the little endian. */
#define XP_ENDIAN_LITTLE 

#define XP_SIZEOF_CHAR 1
#define XP_SIZEOF_SHORT 2
#define XP_SIZEOF_INT 4
#define XP_SIZEOF_LONG 4
#if defined(vax) || defined(__vax)
	#define XP_SIZEOF_LONG_LONG 0
#elif defined(alpha) || defined(__alpha)
	#define XP_SIZEOF_LONG_LONG 8
#elif defined(itanium) || defined(__itanium)
	#define XP_SIZEOF_LONG_LONG 8
#else
	#define XP_SIZEOF_LONG_LONG 0
#endif

#define XP_SIZEOF___INT8 1
#define XP_SIZEOF___INT16 2
#define XP_SIZEOF___INT32 4
#if defined(vax) || defined(__vax)
	#define XP_SIZEOF___INT64 0
#elif defined(alpha) || defined(__alpha)
	#define XP_SIZEOF___INT64 8
#elif defined(itanium) || defined(__itanium)
	#define XP_SIZEOF___INT64 8
#else
	#define XP_SIZEOF___INT64 0
#endif
#define XP_SIZEOF___INT96 0
#define XP_SIZEOF___INT128 0

#if defined(vax) || defined(__vax)
	#define XP_SIZEOF_VOID_P 4
#elif defined(alpha) || defined(__alpha)
	/*#pragma pointer_size 32
	#define XP_SIZEOF_VOID_P 4*/
	#pragma pointer_size 64
	#define XP_SIZEOF_VOID_P 8
#elif defined(itanium) || defined(__itanium)
	/*#pragma pointer_size 32
	#define XP_SIZEOF_VOID_P 4*/
	#pragma pointer_size 64
	#define XP_SIZEOF_VOID_P 8
#else
	#define XP_SIZEOF_VOID_P 0
#endif

#define XP_SIZEOF_FLOAT 4
#define XP_SIZEOF_DOUBLE 8

#if defined(vax) || defined(__vax)
	#define XP_SIZEOF_LONG_DOUBLE 8
#elif defined(alpha) || defined(__alpha)
	#define XP_SIZEOF_LONG_DOUBLE 16
#elif defined(itanium) || defined(__itanium)
	#define XP_SIZEOF_LONG_DOUBLE 16
#else
	#define XP_SIZEOF_LONG_DOUBLE 0
#endif
