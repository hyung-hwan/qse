/*
 * $Id: macros.h,v 1.28 2005-07-21 15:31:25 bacon Exp $
 */

#ifndef _XP_MACROS_H_
#define _XP_MACROS_H_

#include <xp/types.h>

#ifdef __cplusplus
	#define XP_NULL ((xp_uint_t)0)
#else
	#define XP_NULL ((void*)0)
#endif

#define XP_CHAR_EOF  ((xp_cint_t)-1)

#define xp_sizeof(n)   (sizeof(n))
#define xp_countof(n)  (sizeof(n) / sizeof(n[0]))
#define xp_offsetof(type,member) ((xp_size_t)&((type*)0)->member)

#if defined(_WIN32) && defined(XP_CHAR_IS_WCHAR) && !defined(__LCC__)
    #define xp_main wmain
#elif defined(XP_CHAR_IS_MCHAR)
    #define xp_main main
#endif

#define XP_TYPE_IS_SIGNED(type) (((type)0) > ((type)-1))
#define XP_TYPE_IS_UNSIGNED(type) (((type)0) < ((type)-1))
#define XP_TYPE_MAX(type) \
	((XP_TYPE_IS_SIGNED(type)? (type)~((type)1 << (xp_sizeof(type) * 8 - 1)): (type)(~(type)0)))
#define XP_TYPE_MIN(type) \
	((XP_TYPE_IS_SIGNED(type)? (type)((type)1 << (xp_sizeof(type) * 8 - 1)): (type)0))

#define XP_NUM_IS_POWOF2(x) (((x) & ((x) - 1)) == 0)
#define XP_SWAP(x,y) ((x) ^= (y) ^= (x) ^= (y))
#define XP_ABS(x) ((x) < 0? -(x): (x))

#define XP_LOOP_CONTINUE(id) goto __loop_ ## id ## _begin__;
#define XP_LOOP_BREAK(id)    goto __loop_ ## id ## _end__;
#define XP_LOOP_BEGIN(id)    __loop_ ## id ## _begin__: {
#define XP_LOOP_END(id)      XP_LOOP_CONTINUE(id) } __loop_ ## id ## _end__:;

#define XP_REPEAT(n,blk) \
	do { \
		xp_size_t __xp_repeat_x1 = (xp_size_t)(n); \
		xp_size_t __xp_repeat_x2 = __xp_repeat_x1 >> 4; \
		__xp_repeat_x1 &= 15; \
		while (__xp_repeat_x1-- > 0) { blk; } \
		while (__xp_repeat_x2-- > 0) { \
			blk; blk; blk; blk; blk; blk; blk; blk; \
			blk; blk; blk; blk; blk; blk; blk; blk; \
		} \
	} while (0);


#define XP_MQUOTE(val)  #val
#define XP_MCHAR(ch)    ((xp_mchar_t)ch)
#define XP_MTEXT(txt)   ((const xp_mchar_t*)txt)

/* TODO: if the compiler doesn't have the built-in wchar_t support
 *       XP_WCHAR & XP_WTEXT must be defined differently.
#define XP_WCHAR(ch) ((xp_wchar_t)ch)
#define XP_WTEXT(txt) don't know yet... may have to call a function?
 */
#define XP_WQUOTE(val)  ((const xp_char_t*)L###val)
#define XP_WCHAR(ch)    ((xp_wchar_t)L##ch)
#define XP_WTEXT(txt)   ((const xp_wchar_t*)L##txt)

#if defined(XP_CHAR_IS_MCHAR)
	#define XP_CHAR(ch)   XP_MCHAR(ch)
	#define XP_TEXT(txt)  XP_MTEXT(txt)
	#define XP_QUOTE(val) XP_MQUOTE(val)
#else
	#define XP_CHAR(ch)   XP_WCHAR(ch)
	#define XP_TEXT(txt)  XP_WTEXT(txt)
	#define XP_QUOTE(val) XP_WQUOTE(val)
#endif

#endif
