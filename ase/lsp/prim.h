/*
 * $Id: prim.h,v 1.10 2006-10-29 13:00:39 bacon Exp $
 */

#ifndef _ASE_LSP_PRIM_H_
#define _ASE_LSP_PRIM_H_

#include <ase/lsp/lsp.h>

#ifdef __cplusplus
extern "C" {
#endif

ase_lsp_obj_t* ase_lsp_prim_abort (ase_lsp_t* lsp, ase_lsp_obj_t* args);
ase_lsp_obj_t* ase_lsp_prim_eval  (ase_lsp_t* lsp, ase_lsp_obj_t* args);
ase_lsp_obj_t* ase_lsp_prim_prog1 (ase_lsp_t* lsp, ase_lsp_obj_t* args);
ase_lsp_obj_t* ase_lsp_prim_progn (ase_lsp_t* lsp, ase_lsp_obj_t* args);
ase_lsp_obj_t* ase_lsp_prim_gc    (ase_lsp_t* lsp, ase_lsp_obj_t* args);
ase_lsp_obj_t* ase_lsp_prim_cond  (ase_lsp_t* lsp, ase_lsp_obj_t* args);
ase_lsp_obj_t* ase_lsp_prim_if    (ase_lsp_t* lsp, ase_lsp_obj_t* args);
ase_lsp_obj_t* ase_lsp_prim_while (ase_lsp_t* lsp, ase_lsp_obj_t* args);

ase_lsp_obj_t* ase_lsp_prim_car   (ase_lsp_t* lsp, ase_lsp_obj_t* args);
ase_lsp_obj_t* ase_lsp_prim_cdr   (ase_lsp_t* lsp, ase_lsp_obj_t* args);
ase_lsp_obj_t* ase_lsp_prim_cons  (ase_lsp_t* lsp, ase_lsp_obj_t* args);
ase_lsp_obj_t* ase_lsp_prim_set   (ase_lsp_t* lsp, ase_lsp_obj_t* args);
ase_lsp_obj_t* ase_lsp_prim_setq  (ase_lsp_t* lsp, ase_lsp_obj_t* args);
ase_lsp_obj_t* ase_lsp_prim_quote (ase_lsp_t* lsp, ase_lsp_obj_t* args);
ase_lsp_obj_t* ase_lsp_prim_defun (ase_lsp_t* lsp, ase_lsp_obj_t* args);
ase_lsp_obj_t* ase_lsp_prim_demac (ase_lsp_t* lsp, ase_lsp_obj_t* args);
ase_lsp_obj_t* ase_lsp_prim_let   (ase_lsp_t* lsp, ase_lsp_obj_t* args);
ase_lsp_obj_t* ase_lsp_prim_letx  (ase_lsp_t* lsp, ase_lsp_obj_t* args);

/*---------------------
       prim_compar.c 
  ---------------------*/
ase_lsp_obj_t* ase_lsp_prim_eq (ase_lsp_t* lsp, ase_lsp_obj_t* args);
ase_lsp_obj_t* ase_lsp_prim_ne (ase_lsp_t* lsp, ase_lsp_obj_t* args);
ase_lsp_obj_t* ase_lsp_prim_gt (ase_lsp_t* lsp, ase_lsp_obj_t* args);
ase_lsp_obj_t* ase_lsp_prim_lt (ase_lsp_t* lsp, ase_lsp_obj_t* args);
ase_lsp_obj_t* ase_lsp_prim_ge (ase_lsp_t* lsp, ase_lsp_obj_t* args);
ase_lsp_obj_t* ase_lsp_prim_le (ase_lsp_t* lsp, ase_lsp_obj_t* args);

/*---------------------
       prim_math.c 
  ---------------------*/
ase_lsp_obj_t* ase_lsp_prim_plus  (ase_lsp_t* lsp, ase_lsp_obj_t* args);
ase_lsp_obj_t* ase_lsp_prim_minus (ase_lsp_t* lsp, ase_lsp_obj_t* args);
ase_lsp_obj_t* ase_lsp_prim_mul   (ase_lsp_t* lsp, ase_lsp_obj_t* args);
ase_lsp_obj_t* ase_lsp_prim_div   (ase_lsp_t* lsp, ase_lsp_obj_t* args);
ase_lsp_obj_t* ase_lsp_prim_mod   (ase_lsp_t* lsp, ase_lsp_obj_t* args);

#ifdef __cplusplus
}
#endif

#define ASE_LSP_PRIM_CHECK_ARG_COUNT(lsp,args,min,max) \
{ \
	ase_size_t count; \
	if (ase_lsp_probeargs(lsp->mem, args, &count) == -1) { \
		lsp->errnum = ASE_LSP_ERR_BAD_ARG; \
		return ASE_NULL; \
	} \
	if (count < min) { \
		lsp->errnum = ASE_LSP_ERR_TOO_FEW_ARGS; \
		return ASE_NULL; \
	} \
	if (count > max) { \
		lsp->errnum = ASE_LSP_ERR_TOO_MANY_ARGS; \
		return ASE_NULL; \
	} \
}

#define ASE_LSP_PRIM_MAX_ARG_COUNT ((ase_size_t)~(ase_size_t)0)

#endif
