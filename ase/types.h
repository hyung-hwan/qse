/*
 * $Id: types.h,v 1.13 2005-02-13 16:15:31 bacon Exp $
 */

#ifndef _XP_TYPES_H_
#define _XP_TYPES_H_

#ifndef _WIN32
#include <xp/config.h>
#endif
#include <xp/xp_types.h>

#if defined(XP_HAVE_LONG_LONG)
typedef long long xp_long_t;
typedef unsigned long long xp_ulong_t;
#elif defined(XP_HAVE___INT64)
typedef __int64 xp_long_t;
typedef unsigned __int64 xp_ulong_t;
#else
typedef long xp_long_t;
typedef unsigned long xp_ulong_t;
#endif

#if SIZEOF_VOID_P == SIZEOF_LONG_LONG
typedef long long xp_int_t;
typedef unsigned long long xp_uint_t;
#elif SIZEOF_VOID_P == SIZEOF_LONG
typedef long xp_int_t;
typedef unsigned long xp_uint_t;
#else
typedef int xp_int_t;
typedef unsigned int xp_uint_t;
#endif

#if SIZEOF_CHAR == 8
typedef char xp_int8_t;
typedef unsigned char xp_uint8_t;
#endif

#if SIZEOF_SHORT == 16
typedef short xp_int16_t;
typedef unsigned short xp_uint16_t;
#endif

#if SIZEOF_INT == 32
typedef int xp_int32_t;
typedef unsigned int xp_uint32_t;
#elif SIZEOF_LONG == 32
typedef long xp_int32_t;
typedef unsigned long xp_uint32_t;
#endif

#if SIZEOF_INT = 64
typedef int xp_int64_t;
typedef unsigned int xp_uint64_t;
#elif SIZEOF_LONG == 64
typedef long xp_int64_t;
typedef unsigned long xp_uint64_t;
#elif SIZEOF_LONG_LONG == 64
typedef long long xp_int64_t;
typedef unsigned long long xp_uint64_t;
#endif

typedef xp_uint8_t  xp_byte_t;

typedef int xp_bool_t;
#define xp_true  (0 == 0)
#define xp_false (0 != 0)

#ifdef XP_HAVE_WCHAR_T
// TODO: make it configurable from outside
/*
#define XP_CHAR_IS_MCHAR
typedef xp_mchar_t  xp_char_t;
typedef xp_mcint_t  xp_cint_t;
*/

#define XP_CHAR_IS_WCHAR
typedef xp_wchar_t  xp_char_t;
typedef xp_wcint_t  xp_cint_t;

#else
#define XP_CHAR_IS_MCHAR
typedef xp_mchar_t  xp_char_t;
typedef xp_mcint_t  xp_cint_t;
#endif

#endif
