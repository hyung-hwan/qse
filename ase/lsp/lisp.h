/*
 * $Id: lisp.h,v 1.1 2005-02-04 15:39:11 bacon Exp $
 */

#ifndef _RBL_LISP_H_
#define _RBL_LISP_H_

#include <xp/lisp/types.h>
#include <xp/lisp/token.h>
#include <xp/lisp/object.h>
#include <xp/lisp/memory.h>

// NOTICE: the function of xp_lisp_creader_t must return -1 on error 
//         and 0 on success. the first argument must be set to 
//         RBL_END_CHAR at the end of input.
typedef int (*xp_lisp_creader_t) (xp_lisp_cint*, void*); 

#define RBL_ERR(lsp)          ((lsp)->error)
#define RBL_ERR_NONE          0
#define RBL_ERR_ABORT         1
#define RBL_ERR_END           2
#define RBL_ERR_MEM           3
#define RBL_ERR_READ          4
#define RBL_ERR_SYNTAX        5
#define RBL_ERR_BAD_ARG       6
#define RBL_ERR_WRONG_ARG     7
#define RBL_ERR_TOO_FEW_ARGS  8
#define RBL_ERR_TOO_MANY_ARGS 9
#define RBL_ERR_UNDEF_FUNC    10 
#define RBL_ERR_BAD_FUNC      11  
#define RBL_ERR_DUP_FORMAL    12
#define RBL_ERR_BAD_SYMBOL    13
#define RBL_ERR_UNDEF_SYMBOL  14
#define RBL_ERR_EMPTY_BODY    15
#define RBL_ERR_BAD_VALUE     16

struct xp_lisp_t 
{
	/* error number */
	int error;
	int opt_undef_symbol;

	/* for read */
	xp_lisp_cint       curc;
	xp_lisp_creader_t  creader;
	void*          creader_extra;
	int            creader_just_set;
	xp_lisp_token_t*   token;

	/* for eval */
	xp_size_t max_eval_depth;  // TODO:....
	xp_size_t eval_depth;

	/* for print */
	FILE*          outstream;

	/* memory manager */
	xp_lisp_mem_t*     mem;
};

typedef struct xp_lisp_t xp_lisp_t;

#ifdef __cplusplus
extern "C" {
#endif

/* lsp.c */
xp_lisp_t* xp_lisp_new   (xp_size_t mem_ubound, xp_size_t mem_ubound_inc);
void   xp_lisp_free  (xp_lisp_t* lsp);
int    xp_lisp_error (xp_lisp_t* lsp, xp_lisp_char* buf, xp_size_t size);

/* read.c */
// TODO: move xp_lisp_set_creader to lsp.c
void       xp_lisp_set_creader (xp_lisp_t* lsp, xp_lisp_creader_t func, void* extra);
xp_lisp_obj_t* xp_lisp_read (xp_lisp_t* lsp);

/* eval.c */
xp_lisp_obj_t* xp_lisp_eval (xp_lisp_t* lsp, xp_lisp_obj_t* obj);

/* print.c */
void xp_lisp_print (xp_lisp_t* lsp, xp_lisp_obj_t* obj);

#ifdef __cplusplus
}
#endif

#endif
