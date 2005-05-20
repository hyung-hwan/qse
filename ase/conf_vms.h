/* TODO: please complete this file */

#define XP_ENDIAN_LITTLE 

#define SIZEOF_CHAR 1
#define SIZEOF_SHORT 2
#define SIZEOF_INT 4

#ifdef _WIN64
	#define SIZEOF_LONG 8
#else
	#define SIZEOF_LONG 4
#endif

#define SIZEOF_LONG_LONG 0

#define SIZEOF___INT8 1
#define SIZEOF___INT16 2
#define SIZEOF___INT32 4
#define SIZEOF___INT64 8
#define SIZEOF___INT128 0

#ifdef _WIN64
	#define SIZEOF_VOID_P 8
#else
	#define SIZEOF_VOID_P 4
#endif

#define SIZEOF_FLOAT 4
#define SIZEOF_DOUBLE 8
#define SIZEOF_LONG_DOUBLE 16
