/*
 * $Id: macros.h,v 1.36 2006-10-22 11:44:46 bacon Exp $
 */

#ifndef _SSE_MACROS_H_
#define _SSE_MACROS_H_

#include <sse/types.h>

#ifdef __cplusplus
	/*#define SSE_NULL ((sse_uint_t)0)*/
	#define SSE_NULL (0)
#else
	#define SSE_NULL ((void*)0)
#endif

#define SSE_CHAR_EOF  ((sse_cint_t)-1)

#define sse_sizeof(n)   (sizeof(n))
#define sse_countof(n)  (sizeof(n) / sizeof(n[0]))
#define sse_offsetof(type,member) ((sse_size_t)&((type*)0)->member)

#if defined(_WIN32) && defined(SSE_CHAR_IS_WCHAR) && !defined(__LCC__)
	#define sse_main wmain
#elif defined(SSE_CHAR_IS_MCHAR)
	#define sse_main main
#endif

#define SSE_TYPE_IS_SIGNED(type) (((type)0) > ((type)-1))
#define SSE_TYPE_IS_UNSIGNED(type) (((type)0) < ((type)-1))
#define SSE_TYPE_MAX(type) \
	((SSE_TYPE_IS_SIGNED(type)? (type)~((type)1 << (sse_sizeof(type) * 8 - 1)): (type)(~(type)0)))
#define SSE_TYPE_MIN(type) \
	((SSE_TYPE_IS_SIGNED(type)? (type)((type)1 << (sse_sizeof(type) * 8 - 1)): (type)0))

#define SSE_NUM_IS_POWOF2(x) (((x) & ((x) - 1)) == 0)
#define SSE_SWAP(x,y,original_type,casting_type) \
	do { \
		x = (original_type)((casting_type)(x) ^ (casting_type)(y)); \
		y = (original_type)((casting_type)(y) ^ (casting_type)(x)); \
		x = (original_type)((casting_type)(x) ^ (casting_type)(y)); \
	} while (0)
#define SSE_ABS(x) ((x) < 0? -(x): (x))

#define SSE_LOOP_CONTINUE(id) goto __loop_ ## id ## _begin__;
#define SSE_LOOP_BREAK(id)    goto __loop_ ## id ## _end__;
#define SSE_LOOP_BEGIN(id)    __loop_ ## id ## _begin__: {
#define SSE_LOOP_END(id)      SSE_LOOP_CONTINUE(id) } __loop_ ## id ## _end__:;

#define SSE_REPEAT(n,blk) \
	do { \
		sse_size_t __sse_repeat_x1 = (sse_size_t)(n); \
		sse_size_t __sse_repeat_x2 = __sse_repeat_x1 >> 4; \
		__sse_repeat_x1 &= 15; \
		while (__sse_repeat_x1-- > 0) { blk; } \
		while (__sse_repeat_x2-- > 0) { \
			blk; blk; blk; blk; blk; blk; blk; blk; \
			blk; blk; blk; blk; blk; blk; blk; blk; \
		} \
	} while (0);

/* obsolete */
#define SSE_MQUOTE_I(val)  #val
#define SSE_MQUOTE(val)    SSE_MQUOTE_I(val)
#define SSE_MCHAR(ch)      ((sse_mchar_t)ch)
#define SSE_MTEXT(txt)     ((const sse_mchar_t*)txt)

/* new short form */
#define SSE_MQ_I(val) #val
#define SSE_MQ(val)   SSE_MQ_I(val)
#define SSE_MC(ch)    ((sse_mchar_t)ch)
#define SSE_MS(str)   ((const sse_mchar_t*)str)
#define SSE_MT(txt)   (txt)

/* TODO: if the compiler doesn't have the built-in wchar_t support
 *       SSE_WCHAR & SSE_WTEXT must be defined differently.
#define SSE_WCHAR(ch) ((sse_wchar_t)ch)
#define SSE_WTEXT(txt) don't know yet... may have to call a function?
 */

/* obsolete */
#define SSE_WQUOTE_I(val) (L###val)
#define SSE_WQUOTE(val)   SSE_WQUOTE_I(val)
#define SSE_WCHAR(ch)     ((sse_wchar_t)L##ch)
#define SSE_WTEXT(txt)    ((const sse_wchar_t*)L##txt)

/* new short form */
#define SSE_WQ_I(val)  (L###val)
#define SSE_WQ(val)    SSE_WQ_I(val)
#define SSE_WC(ch)     ((sse_wchar_t)L##ch)
#define SSE_WS(str)    ((const sse_wchar_t*)L##str)
#define SSE_WT(txt)    (L##txt)

#if defined(SSE_CHAR_IS_MCHAR)
	/* obsolete */
	#define SSE_CHAR(ch)   SSE_MCHAR(ch)
	#define SSE_TEXT(txt)  SSE_MTEXT(txt)
	#define SSE_QUOTE(val) SSE_MQUOTE(val)

	/* new short form */
	#define SSE_C(ch)  SSE_MC(ch)
	#define SSE_S(str) SSE_MS(str)
	#define SSE_T(txt) SSE_MT(txt)
	#define SSE_Q(val) SSE_MQ(val)
#else
	/* obsolete */
	#define SSE_CHAR(ch)   SSE_WCHAR(ch)
	#define SSE_TEXT(txt)  SSE_WTEXT(txt)
	#define SSE_QUOTE(val) SSE_WQUOTE(val)

	/* new short form */
	#define SSE_C(ch)  SSE_WC(ch)
	#define SSE_S(str) SSE_WS(str)
	#define SSE_T(txt) SSE_WT(txt)
	#define SSE_Q(val) SSE_WQ(val)
#endif

#endif
