/*
 * $Id: conf_vms.h,v 1.11 2007-02-19 04:44:13 bacon Exp $
 *
 * {License}
 */

/* all of vax, alpha, ia64 are in the little endian. */
#define ASE_ENDIAN_LITTLE 

/*
 * Refer to the chapter 3 of the following URL for the data sizes.
 * http://h71000.www7.hp.com/commercial/c/docs/6180profile.html
 */

#define ASE_SIZEOF_CHAR 1
#define ASE_SIZEOF_SHORT 2
#define ASE_SIZEOF_INT 4
#define ASE_SIZEOF_LONG 4

#if defined(vax) || defined(__vax)
	#define ASE_SIZEOF_LONG_LONG 0
#elif defined(alpha) || defined(__alpha)
	#define ASE_SIZEOF_LONG_LONG 8
#elif defined(ia64) || defined(__ia64)
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
#elif defined(ia64) || defined(__ia64)
	#define ASE_SIZEOF___INT64 8
#else
	#define ASE_SIZEOF___INT64 0
#endif

#define ASE_SIZEOF___INT96 0
#define ASE_SIZEOF___INT128 0

#if defined(vax) || defined(__vax)
	#define ASE_SIZEOF_VOID_P 4
#elif defined(alpha) || defined(__alpha)
	#if __INITIAL_POINTER_SIZE==64
		#pragma pointer_size 64
		#define ASE_SIZEOF_VOID_P 8
	#elif __INITIAL_POINTER_SIZE==32
		#pragma pointer_size 32
		#define ASE_SIZEOF_VOID_P 4
	#elif __INITIAL_POINTER_SIZE==0
		#define ASE_SIZEOF_VOID_P 4
	#else
		#error "unsupported initial pointer size"
	#endif
#elif defined(ia64) || defined(__ia64)
	#if __INITIAL_POINTER_SIZE==64
		#pragma pointer_size 64
		#define ASE_SIZEOF_VOID_P 8
	#elif __INITIAL_POINTER_SIZE==32 
		#pragma pointer_size 32
		#define ASE_SIZEOF_VOID_P 4
	#elif __INITIAL_POINTER_SIZE==0
		#define ASE_SIZEOF_VOID_P 4
	#else
		#error "unsupported initial pointer size"
	#endif
#else
	#error "unsupported architecture"
#endif

#define ASE_SIZEOF_FLOAT 4
#define ASE_SIZEOF_DOUBLE 8

#if defined(vax) || defined(__vax)
	#define ASE_SIZEOF_LONG_DOUBLE 8
#elif defined(alpha) || defined(__alpha)
	#define ASE_SIZEOF_LONG_DOUBLE 16
#elif defined(ia64) || defined(__ia64)
	#define ASE_SIZEOF_LONG_DOUBLE 16
#else
	#define ASE_SIZEOF_LONG_DOUBLE 0
#endif

#define ASE_SIZEOF_WCHAR_T 4
