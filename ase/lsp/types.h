/*
 * $Id: types.h,v 1.2 2005-02-04 16:00:37 bacon Exp $
 */

#ifndef _XP_LISP_TYPES_H_
#define _XP_LISP_TYPES_H_

#include <xp/types.h>
#include <xp/macros.h>

typedef xp_char_t xp_lisp_char;
typedef xp_cint_t xp_lisp_cint;
typedef int     xp_lisp_int;
typedef float   xp_lisp_float;

#define XP_LISP_CHAR(x)  XP_CHAR(x)
#define XP_LISP_TEXT(x)  XP_TEXT(x)
#define XP_LISP_CHAR_END XP_EOF

#define xp_lisp_ensure(x) XP_ENSURE(x)
#define xp_lisp_assert(x) XP_ASSERT(x)

#endif
