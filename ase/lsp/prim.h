/*
 * $Id: prim.h,v 1.7 2006-10-23 14:42:38 bacon Exp $
 */

#ifndef _SSE_LSP_PRIM_H_
#define _SSE_LSP_PRIM_H_

#include <sse/lsp/lsp.h>

#ifdef __cplusplus
extern "C" {
#endif

sse_lsp_obj_t* sse_lsp_prim_abort (sse_lsp_t* lsp, sse_lsp_obj_t* args);
sse_lsp_obj_t* sse_lsp_prim_eval  (sse_lsp_t* lsp, sse_lsp_obj_t* args);
sse_lsp_obj_t* sse_lsp_prim_prog1 (sse_lsp_t* lsp, sse_lsp_obj_t* args);
sse_lsp_obj_t* sse_lsp_prim_progn (sse_lsp_t* lsp, sse_lsp_obj_t* args);
sse_lsp_obj_t* sse_lsp_prim_gc    (sse_lsp_t* lsp, sse_lsp_obj_t* args);
sse_lsp_obj_t* sse_lsp_prim_cond  (sse_lsp_t* lsp, sse_lsp_obj_t* args);
sse_lsp_obj_t* sse_lsp_prim_if    (sse_lsp_t* lsp, sse_lsp_obj_t* args);
sse_lsp_obj_t* sse_lsp_prim_while (sse_lsp_t* lsp, sse_lsp_obj_t* args);

sse_lsp_obj_t* sse_lsp_prim_car   (sse_lsp_t* lsp, sse_lsp_obj_t* args);
sse_lsp_obj_t* sse_lsp_prim_cdr   (sse_lsp_t* lsp, sse_lsp_obj_t* args);
sse_lsp_obj_t* sse_lsp_prim_cons  (sse_lsp_t* lsp, sse_lsp_obj_t* args);
sse_lsp_obj_t* sse_lsp_prim_set   (sse_lsp_t* lsp, sse_lsp_obj_t* args);
sse_lsp_obj_t* sse_lsp_prim_setq  (sse_lsp_t* lsp, sse_lsp_obj_t* args);
sse_lsp_obj_t* sse_lsp_prim_quote (sse_lsp_t* lsp, sse_lsp_obj_t* args);
sse_lsp_obj_t* sse_lsp_prim_defun (sse_lsp_t* lsp, sse_lsp_obj_t* args);
sse_lsp_obj_t* sse_lsp_prim_demac (sse_lsp_t* lsp, sse_lsp_obj_t* args);
sse_lsp_obj_t* sse_lsp_prim_let   (sse_lsp_t* lsp, sse_lsp_obj_t* args);
sse_lsp_obj_t* sse_lsp_prim_letx  (sse_lsp_t* lsp, sse_lsp_obj_t* args);

/*---------------------
       prim_compar.c 
  ---------------------*/
sse_lsp_obj_t* sse_lsp_prim_eq (sse_lsp_t* lsp, sse_lsp_obj_t* args);
sse_lsp_obj_t* sse_lsp_prim_ne (sse_lsp_t* lsp, sse_lsp_obj_t* args);
sse_lsp_obj_t* sse_lsp_prim_gt (sse_lsp_t* lsp, sse_lsp_obj_t* args);
sse_lsp_obj_t* sse_lsp_prim_lt (sse_lsp_t* lsp, sse_lsp_obj_t* args);
sse_lsp_obj_t* sse_lsp_prim_ge (sse_lsp_t* lsp, sse_lsp_obj_t* args);
sse_lsp_obj_t* sse_lsp_prim_le (sse_lsp_t* lsp, sse_lsp_obj_t* args);

/*---------------------
       prim_math.c 
  ---------------------*/
sse_lsp_obj_t* sse_lsp_prim_plus     (sse_lsp_t* lsp, sse_lsp_obj_t* args);
sse_lsp_obj_t* sse_lsp_prim_minus    (sse_lsp_t* lsp, sse_lsp_obj_t* args);
sse_lsp_obj_t* sse_lsp_prim_multiply (sse_lsp_t* lsp, sse_lsp_obj_t* args);
sse_lsp_obj_t* sse_lsp_prim_divide   (sse_lsp_t* lsp, sse_lsp_obj_t* args);
sse_lsp_obj_t* sse_lsp_prim_modulus  (sse_lsp_t* lsp, sse_lsp_obj_t* args);

#ifdef __cplusplus
}
#endif

#define SSE_LSP_PRIM_CHECK_ARG_COUNT(lsp,args,min,max) \
{ \
	sse_size_t count; \
	if (sse_lsp_probe_args(lsp->mem, args, &count) == -1) { \
		lsp->errnum = SSE_LSP_ERR_BAD_ARG; \
		return SSE_NULL; \
	} \
	if (count < min) { \
		lsp->errnum = SSE_LSP_ERR_TOO_FEW_ARGS; \
		return SSE_NULL; \
	} \
	if (count > max) { \
		lsp->errnum = SSE_LSP_ERR_TOO_MANY_ARGS; \
		return SSE_NULL; \
	} \
}

#define SSE_LSP_PRIM_MAX_ARG_COUNT ((sse_size_t)~(sse_size_t)0)

#endif
