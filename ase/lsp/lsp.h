/*
 * $Id: lsp.h,v 1.2 2005-09-17 17:50:45 bacon Exp $
 */

#ifndef _XP_LSP_LSP_H_
#define _XP_LSP_LSP_H_

/* 
 * HEADER: xp_lsp_t
 *   A lisp-like embeddable language processor is provied for application
 *   development that requires scripting.
 *
 */

#include <xp/lsp/types.h>
#include <xp/lsp/token.h>
#include <xp/lsp/object.h>
#include <xp/lsp/memory.h>

#include <xp/bas/stdio.h> // TODO: may have to remove dependency on stdio?

// NOTICE: the function of xp_lsp_creader_t must return -1 on error 
//         and 0 on success. the first argument must be set to 
//         XP_LISP_END_CHAR at the end of input.
typedef int (*xp_lsp_creader_t) (xp_cint_t*, void*); 

#define XP_LISP_ERR(lsp)          ((lsp)->error)
#define XP_LISP_ERR_NONE          0
#define XP_LISP_ERR_ABORT         1
#define XP_LISP_ERR_END           2
#define XP_LISP_ERR_MEM           3
#define XP_LISP_ERR_READ          4
#define XP_LISP_ERR_SYNTAX        5
#define XP_LISP_ERR_BAD_ARG       6
#define XP_LISP_ERR_WRONG_ARG     7
#define XP_LISP_ERR_TOO_FEW_ARGS  8
#define XP_LISP_ERR_TOO_MANY_ARGS 9
#define XP_LISP_ERR_UNDEF_FUNC    10 
#define XP_LISP_ERR_BAD_FUNC      11  
#define XP_LISP_ERR_DUP_FORMAL    12
#define XP_LISP_ERR_BAD_SYMBOL    13
#define XP_LISP_ERR_UNDEF_SYMBOL  14
#define XP_LISP_ERR_EMPTY_BODY    15
#define XP_LISP_ERR_BAD_VALUE     16

/*
 * STRUCT: xp_lsp_t
 *   Defines the lisp object
 */
struct xp_lsp_t 
{
	/* error number */
	int error;
	int opt_undef_symbol;

	/* for read */
	xp_cint_t curc;
	xp_lsp_creader_t creader;
	void* creader_extra;
	int creader_just_set;
	xp_lsp_token_t*   token;

	/* for eval */
	xp_size_t max_eval_depth;  // TODO:....
	xp_size_t eval_depth;

	/* for print */
	XP_FILE* outstream;

	/* memory manager */
	xp_lsp_mem_t* mem;
	xp_bool_t __malloced;
};

typedef struct xp_lsp_t xp_lsp_t;

#ifdef __cplusplus
extern "C" {
#endif

/*
 * FUNCTION: xp_lsp_open
 *   Instantiate the lisp object
 */
xp_lsp_t* xp_lsp_open (xp_lsp_t* lisp, 
	xp_size_t mem_ubound, xp_size_t mem_ubound_inc);
/*
 * FUNCTION: xp_lsp_close
 *   Destroys the lisp object
 *
 * PARAMETERS:
 *   lsp - the pointer to the lisp object
 */
void xp_lsp_close  (xp_lsp_t* lsp);

int xp_lsp_error (xp_lsp_t* lsp, xp_char_t* buf, xp_size_t size);

/* read.c */
// TODO: move xp_lsp_set_creader to lsp.c
void       xp_lsp_set_creader (xp_lsp_t* lsp, xp_lsp_creader_t func, void* extra);
xp_lsp_obj_t* xp_lsp_read (xp_lsp_t* lsp);

/* eval.c */
xp_lsp_obj_t* xp_lsp_eval (xp_lsp_t* lsp, xp_lsp_obj_t* obj);

/* print.c */
void xp_lsp_print (xp_lsp_t* lsp, xp_lsp_obj_t* obj);

#ifdef __cplusplus
}
#endif

#endif
