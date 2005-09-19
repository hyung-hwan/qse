/*
 * $Id: lsp.h,v 1.14 2005-09-19 14:57:09 bacon Exp $
 */

#ifndef _XP_LSP_LSP_H_
#define _XP_LSP_LSP_H_

/* 
 * HEADER: Lisp
 *   A lisp-like embeddable language processor is provided for application
 *   development that requires simple scripting.
 *
 */

#include <xp/lsp/types.h>
#include <xp/lsp/token.h>
#include <xp/lsp/obj.h>
#include <xp/lsp/mem.h>

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

/*
 * TYPEDEF: xp_lsp_t
 *   Defines a lisp processor type
 */
typedef struct xp_lsp_t xp_lsp_t;

/*
 * TYPEDEF: xp_lsp_io_t
 *   Defines an IO handler type
 */
typedef xp_ssize_t (*xp_lsp_io_t) (
	int cmd, void* arg, xp_char_t* data, xp_size_t count);

enum 
{
	XP_LSP_IO_OPEN,
	XP_LSP_IO_CLOSE,
	XP_LSP_IO_DATA
};

/*
 * TYPEDEF: xp_lsp_prim_t
 *   Defines a primitive type
 */
typedef xp_lsp_obj_t* (*xp_lsp_prim_t) (xp_lsp_t* lsp, xp_lsp_obj_t* obj);


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
	void* input_arg;
	void* output_arg;

	/* memory manager */
	xp_lsp_mem_t* mem;
	xp_bool_t __malloced;
};

#ifdef __cplusplus
extern "C" {
#endif

/*
 * FUNCTION: xp_lsp_open
 *   Instantiates a lisp processor
 *
 * PARAMETERS:
 *   lsp - pointer to lisp processor space or XP_NULL
 *   mem_ubound - memory upper bound
 *   mem_ubound_inc - memory increment 
 */
xp_lsp_t* xp_lsp_open (xp_lsp_t* lisp, 
	xp_size_t mem_ubound, xp_size_t mem_ubound_inc);

/*
 * FUNCTION: xp_lsp_close
 *   Destroys a lisp processor
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
 *   Attaches an input handler function
 *
 * PARAMETERS:
 *   lsp - the lisp processor
 *   input - input handler function
 *   arg - user data to be passed to the input handler
 * 
 * RETURNS:
 *   0 on success, -1 on failure
 */
int xp_lsp_attach_input (xp_lsp_t* lsp, xp_lsp_io_t input, void* arg);

/*
 * FUNCTION: xp_lsp_detach_input
 *   Detaches an input handler function
 * 
 * RETURNS:
 *   0 on success, -1 on failure
 */
int xp_lsp_detach_input (xp_lsp_t* lsp);

/*
 * FUNCTION: xp_lsp_attach_output
 *   Attaches an output handler function
 *
 * PARAMETERS:
 *   lsp - the lisp processor
 *   output - output handler function
 *   arg - user data to be passed to the output handler
 * 
 * RETURNS:
 *   0 on success, -1 on failure
 */
int xp_lsp_attach_output (xp_lsp_t* lsp, xp_lsp_io_t output, void* arg);

/*
 * FUNCTION: xp_lsp_detach_output
 *   Detaches an output handler function
 * 
 * RETURNS:
 *   0 on success, -1 on failure
 */
int xp_lsp_detach_output (xp_lsp_t* lsp);

/*
 * FUNCTION: xp_lsp_read
 *   Reads a lisp expression
 */
xp_lsp_obj_t* xp_lsp_read (xp_lsp_t* lsp);

/*
 * FUNCTION: xp_lsp_eval
 *   Evaluates a lisp object
 */
xp_lsp_obj_t* xp_lsp_eval (xp_lsp_t* lsp, xp_lsp_obj_t* obj);

/*
 * FUNCTION: xp_lsp_print
 *   Prints a lisp object
 */
int xp_lsp_print (xp_lsp_t* lsp, const xp_lsp_obj_t* obj);

/*
 * FUNCTION: xp_lsp_add_prim
 *   Adds a user-defined primitive
 */
int xp_lsp_add_prim (xp_lsp_t* lsp, const xp_char_t* name, xp_lsp_prim_t prim);

/*
 * FUNCTION: xp_lsp_remove_prim
 *   Removes a user-defined primitive
 */
int xp_lsp_remove_prim (xp_lsp_t* lsp, const xp_char_t* name);

#ifdef __cplusplus
}
#endif

#endif
