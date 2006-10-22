/*
 * $Id: conf_dos.h,v 1.6 2006-10-22 11:46:18 bacon Exp $
 */

#if !defined(__LARGE__) && !defined(__HUGE__)
#error this library supports the large and the huge memory models only
#endif

#define SSE_ENDIAN_LITTLE 

#define SSE_SIZEOF_CHAR 1
#define SSE_SIZEOF_SHORT 2
#define SSE_SIZEOF_INT 2
#define SSE_SIZEOF_LONG 4
#define SSE_SIZEOF_LONG_LONG 0

#define SSE_SIZEOF___INT8 0
#define SSE_SIZEOF___INT16 0
#define SSE_SIZEOF___INT32 0
#define SSE_SIZEOF___INT64 0
#define SSE_SIZEOF___INT96 0
#define SSE_SIZEOF___INT128 0

#define SSE_SIZEOF_VOID_P 4

#define SSE_SIZEOF_FLOAT 4
#define SSE_SIZEOF_DOUBLE 8
#define SSE_SIZEOF_LONG_DOUBLE 10  /* turbo c 2.01 */
#define SSE_SIZEOF_WCHAR_T 0 

#define SSE_CHAR_IS_MCHAR
