/*
 * $Id: primitive.h,v 1.1 2005-02-04 15:39:11 bacon Exp $
 */

#ifndef _RBL_PRIM_H_
#define _RBL_PRIM_H_

#include "types.h"
#include "lsp.h"

typedef xp_lisp_obj_t* (*xp_lisp_pimpl_t) (xp_lisp_t*, xp_lisp_obj_t*);

#ifdef __cplusplus
extern "C" {
#endif

xp_lisp_obj_t* xp_lisp_prim_abort (xp_lisp_t*, xp_lisp_obj_t* args);
xp_lisp_obj_t* xp_lisp_prim_eval  (xp_lisp_t*, xp_lisp_obj_t* args);
xp_lisp_obj_t* xp_lisp_prim_prog1 (xp_lisp_t*, xp_lisp_obj_t* args);
xp_lisp_obj_t* xp_lisp_prim_progn (xp_lisp_t*, xp_lisp_obj_t* args);
xp_lisp_obj_t* xp_lisp_prim_gc    (xp_lisp_t*, xp_lisp_obj_t* args);
xp_lisp_obj_t* xp_lisp_prim_cond  (xp_lisp_t*, xp_lisp_obj_t* args);
xp_lisp_obj_t* xp_lisp_prim_if    (xp_lisp_t*, xp_lisp_obj_t* args);
xp_lisp_obj_t* xp_lisp_prim_while (xp_lisp_t*, xp_lisp_obj_t* args);

xp_lisp_obj_t* xp_lisp_prim_car   (xp_lisp_t*, xp_lisp_obj_t* args);
xp_lisp_obj_t* xp_lisp_prim_cdr   (xp_lisp_t*, xp_lisp_obj_t* args);
xp_lisp_obj_t* xp_lisp_prim_cons  (xp_lisp_t*, xp_lisp_obj_t* args);
xp_lisp_obj_t* xp_lisp_prim_set   (xp_lisp_t*, xp_lisp_obj_t* args);
xp_lisp_obj_t* xp_lisp_prim_setq  (xp_lisp_t*, xp_lisp_obj_t* args);
xp_lisp_obj_t* xp_lisp_prim_quote (xp_lisp_t*, xp_lisp_obj_t* args);
xp_lisp_obj_t* xp_lisp_prim_defun (xp_lisp_t*, xp_lisp_obj_t* args);
xp_lisp_obj_t* xp_lisp_prim_demac (xp_lisp_t*, xp_lisp_obj_t* args);
xp_lisp_obj_t* xp_lisp_prim_let   (xp_lisp_t*, xp_lisp_obj_t* args);
xp_lisp_obj_t* xp_lisp_prim_letx  (xp_lisp_t*, xp_lisp_obj_t* args);

xp_lisp_obj_t* xp_lisp_prim_plus  (xp_lisp_t*, xp_lisp_obj_t* args);
xp_lisp_obj_t* xp_lisp_prim_gt    (xp_lisp_t*, xp_lisp_obj_t* args);
xp_lisp_obj_t* xp_lisp_prim_lt    (xp_lisp_t*, xp_lisp_obj_t* args);

#ifdef __cplusplus
}
#endif

#define RBL_PRIM_CHECK_ARG_COUNT(lsp,args,min,max) \
{ \
	xp_size_t count; \
	if (xp_lisp_probe_args(lsp->mem, args, &count) == -1) { \
		lsp->error = RBL_ERR_BAD_ARG; \
		return XP_NULL; \
	} \
	if (count < min) { \
		lsp->error = RBL_ERR_TOO_FEW_ARGS; \
		return XP_NULL; \
	} \
	if (count > max) { \
		lsp->error = RBL_ERR_TOO_MANY_ARGS; \
		return XP_NULL; \
	} \
}

#define RBL_PRIM_MAX_ARG_COUNT ((xp_size_t)~(xp_size_t)0)

#endif
