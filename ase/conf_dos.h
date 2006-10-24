/*
 * $Id: conf_dos.h,v 1.7 2006-10-24 04:30:13 bacon Exp $
 */

#if !defined(__LARGE__) && !defined(__HUGE__)
#error this library supports the large and the huge memory models only
#endif

#define ASE_ENDIAN_LITTLE 

#define ASE_SIZEOF_CHAR 1
#define ASE_SIZEOF_SHORT 2
#define ASE_SIZEOF_INT 2
#define ASE_SIZEOF_LONG 4
#define ASE_SIZEOF_LONG_LONG 0

#define ASE_SIZEOF___INT8 0
#define ASE_SIZEOF___INT16 0
#define ASE_SIZEOF___INT32 0
#define ASE_SIZEOF___INT64 0
#define ASE_SIZEOF___INT96 0
#define ASE_SIZEOF___INT128 0

#define ASE_SIZEOF_VOID_P 4

#define ASE_SIZEOF_FLOAT 4
#define ASE_SIZEOF_DOUBLE 8
#define ASE_SIZEOF_LONG_DOUBLE 10  /* turbo c 2.01 */
#define ASE_SIZEOF_WCHAR_T 0 

#define ASE_CHAR_IS_MCHAR
