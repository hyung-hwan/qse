/*
 * $Id: lsp.h,v 1.5 2005-09-18 10:18:35 bacon Exp $
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

#define XP_LSP_ERR(lsp)  ((lsp)->errnum)
enum 
{
	XP_LSP_ERR_NONE = 0,
	XP_LSP_ERR_ABORT,
	XP_LSP_ERR_END,
	XP_LSP_ERR_MEM,
	XP_LSP_ERR_INPUT_NOT_ATTACHED,
	XP_LSP_ERR_INPUT,
	XP_LSP_ERR_OUTPUT_NOT_ATTACHED,
	XP_LSP_ERR_OUTPUT,
	XP_LSP_ERR_SYNTAX,
	XP_LSP_ERR_BAD_ARG,
	XP_LSP_ERR_WRONG_ARG,
	XP_LSP_ERR_TOO_FEW_ARGS,
	XP_LSP_ERR_TOO_MANY_ARGS,
	XP_LSP_ERR_UNDEF_FUNC,
	XP_LSP_ERR_BAD_FUNC,
	XP_LSP_ERR_DUP_FORMAL,
	XP_LSP_ERR_BAD_SYMBOL,
	XP_LSP_ERR_UNDEF_SYMBOL,
	XP_LSP_ERR_EMPTY_BODY,
	XP_LSP_ERR_BAD_VALUE
};

typedef int (*xp_lsp_io_t) (int cmd, void* owner, void* arg);
enum 
{
	XP_LSP_IO_OPEN,
	XP_LSP_IO_CLOSE,
	XP_LSP_IO_CHAR,
	XP_LSP_IO_STR
};

/*
 * STRUCT: xp_lsp_t
 *   Defines the lisp object
 */
struct xp_lsp_t 
{
	/* error number */
	int errnum;
	int opt_undef_symbol;

	/* for read */
	xp_cint_t curc;
	xp_lsp_token_t token;

	/* for eval */
	xp_size_t max_eval_depth;  // TODO:....
	xp_size_t eval_depth;

	/* io functions */
	xp_lsp_io_t input_func;
	xp_lsp_io_t output_func;

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

/*
 * FUNCTION: xp_lsp_error
 */
int xp_lsp_error (xp_lsp_t* lsp, xp_char_t* buf, xp_size_t size);

/*
 * FUNCTION: xp_lsp_attach_input
 */
int xp_lsp_attach_input (xp_lsp_t* lsp, xp_lsp_io_t input);

/*
 * FUNCTION: xp_lsp_detach_input
 */
int xp_lsp_detach_input (xp_lsp_t* lsp);

/*
 * FUNCTION: xp_lsp_attach_output
 */
int xp_lsp_attach_output (xp_lsp_t* lsp, xp_lsp_io_t output);

/*
 * FUNCTION: xp_lsp_detach_output
 */
int xp_lsp_detach_output (xp_lsp_t* lsp);

/*
 * FUNCTION: xp_lsp_read
 */
xp_lsp_obj_t* xp_lsp_read (xp_lsp_t* lsp);

/*
 * FUNCTION: xp_lsp_eval
 */
xp_lsp_obj_t* xp_lsp_eval (xp_lsp_t* lsp, xp_lsp_obj_t* obj);

/*
 * FUNCTION: xp_lsp_print
 */
int xp_lsp_print (xp_lsp_t* lsp, const xp_lsp_obj_t* obj);

#ifdef __cplusplus
}
#endif

#endif
