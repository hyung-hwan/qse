/*
 * $Id: types.h,v 1.1 2005-02-04 15:39:11 bacon Exp $
 */

#ifndef _RBL_TYPES_H_
#define _RBL_TYPES_H_

#include <rb/types.h>
#include <rb/macros.h>

typedef rb_char xp_lisp_char;
typedef rb_cint xp_lisp_cint;
typedef int     xp_lisp_int;
typedef float   xp_lisp_float;

#define RBL_CHAR(x)  RB_CHAR(x)
#define RBL_TEXT(x)  RB_TEXT(x)
#define RBL_CHAR_END RB_EOF

#define xp_lisp_ensure(x) RB_ENSURE(x)
#define xp_lisp_assert(x) RB_ASSERT(x)

#endif
