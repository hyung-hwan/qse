/*
 * $Id: lsp.h,v 1.21 2006-10-22 13:10:46 bacon Exp $
 */

#ifndef _SSE_LSP_LSP_H_
#define _SSE_LSP_LSP_H_

/* 
 * HEADER: Lisp
 *   A lisp-like embeddable language processor is provided for application
 *   development that requires simple scripting.
 *
 */

#include <sse/lsp/types.h>
#include <sse/lsp/token.h>
#include <sse/lsp/obj.h>
#include <sse/lsp/mem.h>

#define SSE_LSP_ERR(lsp)  ((lsp)->errnum)
enum 
{
	SSE_LSP_ERR_NONE = 0,
	SSE_LSP_ERR_ABORT,
	SSE_LSP_ERR_END,
	SSE_LSP_ERR_MEMORY,
	SSE_LSP_ERR_INPUT_NOT_ATTACHED,
	SSE_LSP_ERR_INPUT,
	SSE_LSP_ERR_OUTPUT_NOT_ATTACHED,
	SSE_LSP_ERR_OUTPUT,
	SSE_LSP_ERR_SYNTAX,
	SSE_LSP_ERR_BAD_ARG,
	SSE_LSP_ERR_WRONG_ARG,
	SSE_LSP_ERR_TOO_FEW_ARGS,
	SSE_LSP_ERR_TOO_MANY_ARGS,
	SSE_LSP_ERR_UNDEF_FUNC,
	SSE_LSP_ERR_BAD_FUNC,
	SSE_LSP_ERR_DUP_FORMAL,
	SSE_LSP_ERR_BAD_SYMBOL,
	SSE_LSP_ERR_UNDEF_SYMBOL,
	SSE_LSP_ERR_EMPTY_BODY,
	SSE_LSP_ERR_BAD_VALUE,
	SSE_LSP_ERR_DIVIDE_BY_ZERO
};

/*
 * TYPE: sse_lsp_t
 *   Defines a lisp processor type
 */
typedef struct sse_lsp_t sse_lsp_t;

/*
 * TYPE: sse_lsp_io_t
 *   Defines an IO handler type
 */
typedef sse_ssize_t (*sse_lsp_io_t) (
	int cmd, void* arg, sse_char_t* data, sse_size_t count);

enum 
{
	SSE_LSP_IO_OPEN,
	SSE_LSP_IO_CLOSE,
	SSE_LSP_IO_DATA
};

/*
 * TYPEDEF: sse_lsp_prim_t
 *   Defines a primitive type
 */
typedef sse_lsp_obj_t* (*sse_lsp_prim_t) (sse_lsp_t* lsp, sse_lsp_obj_t* obj);

struct sse_lsp_t 
{
	/* error number */
	int errnum;
	int opt_undef_symbol;

	/* for read */
	sse_cint_t curc;
	sse_lsp_token_t token;

	/* io functions */
	sse_lsp_io_t input_func;
	sse_lsp_io_t output_func;
	void* input_arg;
	void* output_arg;

	/* security options */
	sse_size_t max_eval_depth;
	sse_size_t cur_eval_depth;

	/* memory manager */
	sse_lsp_mem_t* mem;
	sse_bool_t __dynamic;
};

#ifdef __cplusplus
extern "C" {
#endif

/*
 * FUNCTION: sse_lsp_open
 *   Instantiates a lisp processor
 *
 * PARAMETERS:
 *   lsp - pointer to lisp processor space or SSE_NULL
 *   mem_ubound - memory upper bound
 *   mem_ubound_inc - memory increment 
 */
sse_lsp_t* sse_lsp_open (sse_lsp_t* lsp, 
	sse_size_t mem_ubound, sse_size_t mem_ubound_inc);

/*
 * FUNCTION: sse_lsp_close
 *   Destroys a lisp processor
 *
 * PARAMETERS:
 *   lsp - the pointer to the lisp object
 */
void sse_lsp_close (sse_lsp_t* lsp);

/*
 * FUNCTION: sse_lsp_error
 */
int sse_lsp_error (sse_lsp_t* lsp, sse_char_t* buf, sse_size_t size);

/*
 * FUNCTION: sse_lsp_attach_input
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
int sse_lsp_attach_input (sse_lsp_t* lsp, sse_lsp_io_t input, void* arg);

/*
 * FUNCTION: sse_lsp_detach_input
 *   Detaches an input handler function
 * 
 * RETURNS:
 *   0 on success, -1 on failure
 */
int sse_lsp_detach_input (sse_lsp_t* lsp);

/*
 * FUNCTION: sse_lsp_attach_output
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
int sse_lsp_attach_output (sse_lsp_t* lsp, sse_lsp_io_t output, void* arg);

/*
 * FUNCTION: sse_lsp_detach_output
 *   Detaches an output handler function
 * 
 * RETURNS:
 *   0 on success, -1 on failure
 */
int sse_lsp_detach_output (sse_lsp_t* lsp);

/*
 * FUNCTION: sse_lsp_read
 *   Reads a lisp expression
 */
sse_lsp_obj_t* sse_lsp_read (sse_lsp_t* lsp);

/*
 * FUNCTION: sse_lsp_eval
 *   Evaluates a lisp object
 */
sse_lsp_obj_t* sse_lsp_eval (sse_lsp_t* lsp, sse_lsp_obj_t* obj);

/*
 * FUNCTION: sse_lsp_print
 *   Prints a lisp object
 */
int sse_lsp_print (sse_lsp_t* lsp, const sse_lsp_obj_t* obj);

/*
 * FUNCTION: sse_lsp_add_prim
 *   Adds a user-defined primitive
 */
int sse_lsp_add_prim (sse_lsp_t* lsp, const sse_char_t* name, sse_lsp_prim_t prim);

/*
 * FUNCTION: sse_lsp_remove_prim
 *   Removes a user-defined primitive
 */
int sse_lsp_remove_prim (sse_lsp_t* lsp, const sse_char_t* name);

#ifdef __cplusplus
}
#endif

#endif
