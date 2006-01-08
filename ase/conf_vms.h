/* TODO: please complete the itanium portion */

/* both vax and alpha are in the little endian. */
#define XP_ENDIAN_LITTLE 

#define SIZEOF_CHAR 1
#define SIZEOF_SHORT 2
#define SIZEOF_INT 4
#define SIZEOF_LONG 4
#if defined(vax) || defined(__vax)
	#define SIZEOF_LONG_LONG 0
#elif defined(alpha) || defined(__alpha)
	#define SIZEOF_LONG_LONG 8
#elif defined(itanium) || defined(__itanium)
	#define SIZEOF_LONG_LONG 8
#else
	#define SIZEOF_LONG_LONG 0
#endif

#define SIZEOF___INT8 1
#define SIZEOF___INT16 2
#define SIZEOF___INT32 4
#if defined(vax) || defined(__vax)
	#define SIZEOF___INT64 0
#elif defined(alpha) || defined(__alpha)
	#define SIZEOF___INT64 8
#elif defined(itanium) || defined(__itanium)
	#define SIZEOF___INT64 8
#else
	#define SIZEOF___INT64 0
#endif
#define SIZEOF___INT128 0

#if defined(vax) || defined(__vax)
	#define SIZEOF_VOID_P 4
#elif defined(alpha) || defined(__alpha)
	/*#pragma pointer_size 32
	#define SIZEOF_VOID_P 4*/
	#pragma pointer_size 64
	#define SIZEOF_VOID_P 8
#elif defined(itanium) || defined(__itanium)
	/*#pragma pointer_size 32
	#define SIZEOF_VOID_P 4*/
	#pragma pointer_size 64
	#define SIZEOF_VOID_P 8
#else
	#define SIZEOF_VOID_P 0
#endif

#define SIZEOF_FLOAT 4
#define SIZEOF_DOUBLE 8

#if defined(vax) || defined(__vax)
	#define SIZEOF_LONG_DOUBLE 8
#elif defined(alpha) || defined(__alpha)
	#define SIZEOF_LONG_DOUBLE 16
#elif defined(itanium) || defined(__itanium)
	#define SIZEOF_LONG_DOUBLE 16
#else
	#define SIZEOF_LONG_DOUBLE 0
#endif
