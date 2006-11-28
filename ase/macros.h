/*
 * $Id: macros.h,v 1.39 2006-11-28 11:34:34 bacon Exp $
 */

#ifndef _ASE_MACROS_H_
#define _ASE_MACROS_H_

#include <ase/types.h>

#ifdef __cplusplus
	/*#define ASE_NULL ((ase_uint_t)0)*/
	#define ASE_NULL (0)
#else
	#define ASE_NULL ((void*)0)
#endif

#define ASE_CHAR_EOF  ((ase_cint_t)-1)

#define ase_sizeof(n)   (sizeof(n))
#define ase_countof(n)  (sizeof(n) / sizeof(n[0]))
#define ase_offsetof(type,member) ((ase_size_t)&((type*)0)->member)

#if defined(_WIN32) && defined(ASE_CHAR_IS_WCHAR) && !defined(__LCC__)
	#define ase_main wmain
#elif defined(ASE_CHAR_IS_MCHAR)
	#define ase_main main
#endif

#define ASE_TYPE_IS_SIGNED(type) (((type)0) > ((type)-1))
#define ASE_TYPE_IS_UNSIGNED(type) (((type)0) < ((type)-1))
#define ASE_TYPE_MAX(type) \
	((ASE_TYPE_IS_SIGNED(type)? (type)~((type)1 << (ase_sizeof(type) * 8 - 1)): (type)(~(type)0)))
#define ASE_TYPE_MIN(type) \
	((ASE_TYPE_IS_SIGNED(type)? (type)((type)1 << (ase_sizeof(type) * 8 - 1)): (type)0))

#define ASE_NUM_IS_POWOF2(x) (((x) & ((x) - 1)) == 0)
#define ASE_SWAP(x,y,original_type,casting_type) \
	do { \
		x = (original_type)((casting_type)(x) ^ (casting_type)(y)); \
		y = (original_type)((casting_type)(y) ^ (casting_type)(x)); \
		x = (original_type)((casting_type)(x) ^ (casting_type)(y)); \
	} while (0)
#define ASE_ABS(x) ((x) < 0? -(x): (x))

#define ASE_LOOP_CONTINUE(id) goto __loop_ ## id ## _begin__;
#define ASE_LOOP_BREAK(id)    goto __loop_ ## id ## _end__;
#define ASE_LOOP_BEGIN(id)    __loop_ ## id ## _begin__: {
#define ASE_LOOP_END(id)      ASE_LOOP_CONTINUE(id) } __loop_ ## id ## _end__:;

#define ASE_REPEAT(n,blk) \
	do { \
		ase_size_t __ase_repeat_x1 = (ase_size_t)(n); \
		ase_size_t __ase_repeat_x2 = __ase_repeat_x1 >> 4; \
		__ase_repeat_x1 &= 15; \
		while (__ase_repeat_x1-- > 0) { blk; } \
		while (__ase_repeat_x2-- > 0) { \
			blk; blk; blk; blk; blk; blk; blk; blk; \
			blk; blk; blk; blk; blk; blk; blk; blk; \
		} \
	} while (0);

#define ASE_MQ_I(val) #val
#define ASE_MQ(val)   ASE_MQ_I(val)
#define ASE_MC(ch)    ((ase_mchar_t)ch)
#define ASE_MS(str)   ((const ase_mchar_t*)str)
#define ASE_MT(txt)   (txt)

/* TODO: if the compiler doesn't have the built-in wchar_t support
 *       ASE_WCHAR & ASE_WTEXT must be defined differently.
#define ASE_WCHAR(ch) ((ase_wchar_t)ch)
#define ASE_WTEXT(txt) don't know yet... may have to call a function?
 */

/* new short form */
#define ASE_WQ_I(val)  (L ## #val)
#define ASE_WQ(val)    ASE_WQ_I(val)
#define ASE_WC(ch)     ((ase_wchar_t)L ## ch)
#define ASE_WS(str)    ((const ase_wchar_t*)L ## str)
#define ASE_WT(txt)    (L ## txt)

#if defined(ASE_CHAR_IS_MCHAR)
	#define ASE_C(ch)  ASE_MC(ch)
	#define ASE_S(str) ASE_MS(str)
	#define ASE_T(txt) ASE_MT(txt)
	#define ASE_Q(val) ASE_MQ(val)
#else
	/* new short form */
	#define ASE_C(ch)  ASE_WC(ch)
	#define ASE_S(str) ASE_WS(str)
	#define ASE_T(txt) ASE_WT(txt)
	#define ASE_Q(val) ASE_WQ(val)
#endif

#endif
