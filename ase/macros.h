/*
 * $Id: macros.h,v 1.10 2005-02-05 06:30:24 bacon Exp $
 */

#ifndef _XP_MACROS_H_
#define _XP_MACROS_H_

#include <xp/types.h>

#define XP_NULL ((void*)0)
#define XP_EOF  ((xp_cint_t)-1)

#define xp_sizeof(n)   (sizeof(n))
#define xp_countof(n)  (sizeof(n) / sizeof(n[0]))

#if defined(_WIN32)
    #define xp_main _tmain
#elif defined(XP_CHAR_IS_MCHAR)
    #define xp_main main
#endif

#define XP_LOOP_CONTINUE(id) goto __loop_ ## id ## _begin__;
#define XP_LOOP_BREAK(id)    goto __loop_ ## id ## _end__;
#define XP_LOOP_BEGIN(id)    __loop_ ## id ## _begin__: {
#define XP_LOOP_END(id)      XP_LOOP_CONTINUE(id) } __loop_ ## id ## _end__:;

#define XP_QUOTE(val)   __XP_QUOTE(val)
#define __XP_QUOTE(val) #val

//#define XP_MCHAR(ch)    ((xp_mchar_t)ch)
//#define XP_MTEXT(txt)   ((const xp_mchar_t*)txt)
#define XP_MCHAR(ch)    ch
#define XP_MTEXT(txt)   txt

/* TODO: if the compiler doesn't have the built-in wchar_t support
 *       XP_WCHAR & XP_WTEXT must be defined differently.
#define XP_WCHAR(ch) ((xp_wchar_t)ch)
#define XP_WTEXT(txt) don't know yet... may have to call a function?
 */
#define XP_WCHAR(ch)    L##ch
#define XP_WTEXT(txt)   L##txt

#if defined(XP_CHAR_IS_MCHAR)
	/*
	#define XP_CHAR(ch)   XP_MCHAR(ch)
	#define XP_TEXT(txt)  XP_MTEXT(txt)
	*/
	#define XP_CHAR(ch)    ch
	#define XP_TEXT(txt)   txt
#else
	/*
	#define XP_CHAR(ch)   XP_WCHAR(ch)
	#define XP_TEXT(txt)  XP_WTEXT(txt)
	*/
	#define XP_CHAR(ch)    L##ch
	#define XP_TEXT(txt)   L##txt
#endif

#endif
