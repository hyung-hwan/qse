/*
 * $Id: macros.h,v 1.1 2004-12-18 09:16:31 bacon Exp $
 */

#ifndef _RB_MACROS_H_
#define _RB_MACROS_H_

#include <rb/types.h>

#define RB_NULL ((void*)0)
#define RB_EOF  ((rb_cint_t)-1)

#define rb_sizeof(n)   (sizeof(n))
#define rb_countof(n)  (sizeof(n) / sizeof(n[0]))

#if defined(_WIN32)
    #define rb_main _tmain
#else
    #define rb_main main
#endif

#define RB_LOOP_CONTINUE(id) goto __loop_ ## id ## _begin__;
#define RB_LOOP_BREAK(id)    goto __loop_ ## id ## _end__;
#define RB_LOOP_BEGIN(id)    __loop_ ## id ## _begin__: {
#define RB_LOOP_END(id)      RB_LOOP_CONTINUE(id) } __loop_ ## id ## _end__:;

#define RB_QUOTE(val)   __RB_QUOTE(val)
#define __RB_QUOTE(val) #val

#define __RB_CHAR(quote)  quote
#define __RB_TEXT(quote)  quote
#define RB_MCHAR(quote)   quote
#define RB_MTEXT(quote)   quote

/*
#define __RB_CHAR(quote)  L##quote
#define __RB_TEXT(quote)  L##quote
#define RB_WCHAR(quote)   L##quote
#define RB_WTEXT(quote)   L##quote
*/

#define RB_CHAR(quote) __RB_CHAR(quote)
#define RB_TEXT(quote) __RB_TEXT(quote)

#endif
