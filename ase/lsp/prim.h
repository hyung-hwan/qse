/*
 * $Id: prim.h,v 1.1.1.1 2007/03/28 14:05:24 bacon Exp $
 *
 * {License}
 */

#ifndef _ASE_LSP_PRIM_H_
#define _ASE_LSP_PRIM_H_

#ifndef _ASE_LSP_LSP_H_
#error Never include this file directly. Include <ase/lsp/lsp.h> instead
#endif

#ifdef __cplusplus
extern "C" {
#endif

ase_lsp_obj_t* ase_lsp_prim_exit   (ase_lsp_t* lsp, ase_lsp_obj_t* args);
ase_lsp_obj_t* ase_lsp_prim_eval   (ase_lsp_t* lsp, ase_lsp_obj_t* args);
ase_lsp_obj_t* ase_lsp_prim_prog1  (ase_lsp_t* lsp, ase_lsp_obj_t* args);
ase_lsp_obj_t* ase_lsp_prim_progn  (ase_lsp_t* lsp, ase_lsp_obj_t* args);
ase_lsp_obj_t* ase_lsp_prim_gc     (ase_lsp_t* lsp, ase_lsp_obj_t* args);
ase_lsp_obj_t* ase_lsp_prim_cond   (ase_lsp_t* lsp, ase_lsp_obj_t* args);
ase_lsp_obj_t* ase_lsp_prim_if     (ase_lsp_t* lsp, ase_lsp_obj_t* args);
ase_lsp_obj_t* ase_lsp_prim_while  (ase_lsp_t* lsp, ase_lsp_obj_t* args);

ase_lsp_obj_t* ase_lsp_prim_car    (ase_lsp_t* lsp, ase_lsp_obj_t* args);
ase_lsp_obj_t* ase_lsp_prim_cdr    (ase_lsp_t* lsp, ase_lsp_obj_t* args);
ase_lsp_obj_t* ase_lsp_prim_cons   (ase_lsp_t* lsp, ase_lsp_obj_t* args);
ase_lsp_obj_t* ase_lsp_prim_length (ase_lsp_t* lsp, ase_lsp_obj_t* args);

ase_lsp_obj_t* ase_lsp_prim_set    (ase_lsp_t* lsp, ase_lsp_obj_t* args);
ase_lsp_obj_t* ase_lsp_prim_setq   (ase_lsp_t* lsp, ase_lsp_obj_t* args);
ase_lsp_obj_t* ase_lsp_prim_quote  (ase_lsp_t* lsp, ase_lsp_obj_t* args);
ase_lsp_obj_t* ase_lsp_prim_defun  (ase_lsp_t* lsp, ase_lsp_obj_t* args);
ase_lsp_obj_t* ase_lsp_prim_demac  (ase_lsp_t* lsp, ase_lsp_obj_t* args);
ase_lsp_obj_t* ase_lsp_prim_let    (ase_lsp_t* lsp, ase_lsp_obj_t* args);
ase_lsp_obj_t* ase_lsp_prim_letx   (ase_lsp_t* lsp, ase_lsp_obj_t* args);
ase_lsp_obj_t* ase_lsp_prim_or     (ase_lsp_t* lsp, ase_lsp_obj_t* args);

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

/*---------------------
       prim_fact.c 
  ---------------------*/
ase_lsp_obj_t* ase_lsp_prim_fact (ase_lsp_t* lsp, ase_lsp_obj_t* args);

#ifdef __cplusplus
}
#endif

#endif
