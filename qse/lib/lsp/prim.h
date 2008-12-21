/*
 * $Id: prim.h 117 2008-03-03 11:20:05Z baconevi $
 *
 * {License}
 */

#ifndef _QSE_LSP_PRIM_H_
#define _QSE_LSP_PRIM_H_

#ifndef _QSE_LSP_LSP_H_
#error Never include this file directly. Include <qse/lsp/lsp.h> instead
#endif

#ifdef __cplusplus
extern "C" {
#endif

qse_lsp_obj_t* qse_lsp_prim_exit   (qse_lsp_t* lsp, qse_lsp_obj_t* args);
qse_lsp_obj_t* qse_lsp_prim_eval   (qse_lsp_t* lsp, qse_lsp_obj_t* args);
qse_lsp_obj_t* qse_lsp_prim_prog1  (qse_lsp_t* lsp, qse_lsp_obj_t* args);
qse_lsp_obj_t* qse_lsp_prim_progn  (qse_lsp_t* lsp, qse_lsp_obj_t* args);
qse_lsp_obj_t* qse_lsp_prim_gc     (qse_lsp_t* lsp, qse_lsp_obj_t* args);
qse_lsp_obj_t* qse_lsp_prim_cond   (qse_lsp_t* lsp, qse_lsp_obj_t* args);
qse_lsp_obj_t* qse_lsp_prim_if     (qse_lsp_t* lsp, qse_lsp_obj_t* args);
qse_lsp_obj_t* qse_lsp_prim_while  (qse_lsp_t* lsp, qse_lsp_obj_t* args);

qse_lsp_obj_t* qse_lsp_prim_car    (qse_lsp_t* lsp, qse_lsp_obj_t* args);
qse_lsp_obj_t* qse_lsp_prim_cdr    (qse_lsp_t* lsp, qse_lsp_obj_t* args);
qse_lsp_obj_t* qse_lsp_prim_cons   (qse_lsp_t* lsp, qse_lsp_obj_t* args);
qse_lsp_obj_t* qse_lsp_prim_length (qse_lsp_t* lsp, qse_lsp_obj_t* args);

qse_lsp_obj_t* qse_lsp_prim_set    (qse_lsp_t* lsp, qse_lsp_obj_t* args);
qse_lsp_obj_t* qse_lsp_prim_setq   (qse_lsp_t* lsp, qse_lsp_obj_t* args);
qse_lsp_obj_t* qse_lsp_prim_quote  (qse_lsp_t* lsp, qse_lsp_obj_t* args);
qse_lsp_obj_t* qse_lsp_prim_defun  (qse_lsp_t* lsp, qse_lsp_obj_t* args);
qse_lsp_obj_t* qse_lsp_prim_demac  (qse_lsp_t* lsp, qse_lsp_obj_t* args);
qse_lsp_obj_t* qse_lsp_prim_let    (qse_lsp_t* lsp, qse_lsp_obj_t* args);
qse_lsp_obj_t* qse_lsp_prim_letx   (qse_lsp_t* lsp, qse_lsp_obj_t* args);
qse_lsp_obj_t* qse_lsp_prim_or     (qse_lsp_t* lsp, qse_lsp_obj_t* args);

/*---------------------
       prim_compar.c 
  ---------------------*/
qse_lsp_obj_t* qse_lsp_prim_eq (qse_lsp_t* lsp, qse_lsp_obj_t* args);
qse_lsp_obj_t* qse_lsp_prim_ne (qse_lsp_t* lsp, qse_lsp_obj_t* args);
qse_lsp_obj_t* qse_lsp_prim_gt (qse_lsp_t* lsp, qse_lsp_obj_t* args);
qse_lsp_obj_t* qse_lsp_prim_lt (qse_lsp_t* lsp, qse_lsp_obj_t* args);
qse_lsp_obj_t* qse_lsp_prim_ge (qse_lsp_t* lsp, qse_lsp_obj_t* args);
qse_lsp_obj_t* qse_lsp_prim_le (qse_lsp_t* lsp, qse_lsp_obj_t* args);

/*---------------------
       prim_math.c 
  ---------------------*/
qse_lsp_obj_t* qse_lsp_prim_plus  (qse_lsp_t* lsp, qse_lsp_obj_t* args);
qse_lsp_obj_t* qse_lsp_prim_minus (qse_lsp_t* lsp, qse_lsp_obj_t* args);
qse_lsp_obj_t* qse_lsp_prim_mul   (qse_lsp_t* lsp, qse_lsp_obj_t* args);
qse_lsp_obj_t* qse_lsp_prim_div   (qse_lsp_t* lsp, qse_lsp_obj_t* args);
qse_lsp_obj_t* qse_lsp_prim_mod   (qse_lsp_t* lsp, qse_lsp_obj_t* args);

/*---------------------
       prim_fact.c 
  ---------------------*/
qse_lsp_obj_t* qse_lsp_prim_fact (qse_lsp_t* lsp, qse_lsp_obj_t* args);

#ifdef __cplusplus
}
#endif

#endif
