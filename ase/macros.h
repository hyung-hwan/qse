/*
 * $Id: macros.h,v 1.3 2005-01-06 15:31:59 bacon Exp $
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
#else
    #define xp_main main
#endif

#define XP_LOOP_CONTINUE(id) goto __loop_ ## id ## _begin__;
#define XP_LOOP_BREAK(id)    goto __loop_ ## id ## _end__;
#define XP_LOOP_BEGIN(id)    __loop_ ## id ## _begin__: {
#define XP_LOOP_END(id)      XP_LOOP_CONTINUE(id) } __loop_ ## id ## _end__:;

#define XP_QUOTE(val)   __XP_QUOTE(val)
#define __XP_QUOTE(val) #val

#ifdef XP_CHAR_IS_MCHAR

#define __XP_CHAR(quote)  quote
#define __XP_TEXT(quote)  quote
#define XP_MCHAR(quote)   quote
#define XP_MTEXT(quote)   quote

#else

#define __XP_CHAR(quote)  L##quote
#define __XP_TEXT(quote)  L##quote
#define XP_WCHAR(quote)   L##quote
#define XP_WTEXT(quote)   L##quote

#endif

#define XP_CHAR(quote) __XP_CHAR(quote)
#define XP_TEXT(quote) __XP_TEXT(quote)

#endif
