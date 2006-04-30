/*
 * $Id: conf_dos.h,v 1.4 2006-04-30 18:05:07 bacon Exp $
 */

#if !defined(__LARGE__) && !defined(__HUGE__)
#error this library supports the large and the huge memory models only
#endif

#define XP_ENDIAN_LITTLE 

#define XP_SIZEOF_CHAR 1
#define XP_SIZEOF_SHORT 2
#define XP_SIZEOF_INT 2
#define XP_SIZEOF_LONG 4
#define XP_SIZEOF_LONG_LONG 0

#define XP_SIZEOF___INT8 0
#define XP_SIZEOF___INT16 0
#define XP_SIZEOF___INT32 0
#define XP_SIZEOF___INT64 0
#define XP_SIZEOF___INT96 0
#define XP_SIZEOF___INT128 0

#define XP_SIZEOF_VOID_P 4

#define XP_SIZEOF_FLOAT 4
#define XP_SIZEOF_DOUBLE 8
#define XP_SIZEOF_LONG_DOUBLE 10  /* turbo c 2.01 */

#define XP_CHAR_IS_MCHAR
