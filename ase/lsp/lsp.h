/*
 * $Id: lsp.h,v 1.23 2006-10-23 14:42:38 bacon Exp $
 */

#ifndef _SSE_LSP_LSP_H_
#define _SSE_LSP_LSP_H_

#include <sse/types.h>
#include <sse/macros.h>

typedef struct sse_lsp_t sse_lsp_t;
typedef struct sse_lsp_obj_t sse_lsp_obj_t;
typedef struct sse_lsp_syscas_t sse_lsp_syscas_t;

typedef sse_ssize_t (*sse_lsp_io_t) (
	int cmd, void* arg, sse_char_t* data, sse_size_t count);

struct sse_lsp_syscas_t
{
	/* memory */
	void* (*malloc) (sse_size_t n, void* custom_data);
	void* (*realloc) (void* ptr, sse_size_t n, void* custom_data);
	void  (*free) (void* ptr, void* custom_data);

	/* character class */
	sse_bool_t (*is_upper)  (sse_cint_t c);
	sse_bool_t (*is_lower)  (sse_cint_t c);
	sse_bool_t (*is_alpha)  (sse_cint_t c);
	sse_bool_t (*is_digit)  (sse_cint_t c);
	sse_bool_t (*is_xdigit) (sse_cint_t c);
	sse_bool_t (*is_alnum)  (sse_cint_t c);
	sse_bool_t (*is_space)  (sse_cint_t c);
	sse_bool_t (*is_print)  (sse_cint_t c);
	sse_bool_t (*is_graph)  (sse_cint_t c);
	sse_bool_t (*is_cntrl)  (sse_cint_t c);
	sse_bool_t (*is_punct)  (sse_cint_t c);
	sse_cint_t (*to_upper)  (sse_cint_t c);
	sse_cint_t (*to_lower)  (sse_cint_t c);

	/* utilities */
	void* (*memcpy)  (void* dst, const void* src, sse_size_t n);
	void* (*memset)  (void* dst, int val, sse_size_t n);

	int (*sprintf) (sse_char_t* buf, sse_size_t size, sse_char_t* fmt, ...);
	int (*dprintf) (sse_char_t* fmt, ...);
	void (*abort) (void);

	void* custom_data;
};

/* io function commands */
enum 
{
	SSE_LSP_IO_OPEN   = 0,
	SSE_LSP_IO_CLOSE  = 1,
	SSE_LSP_IO_READ   = 2,
	SSE_LSP_IO_WRITE  = 3
};

/* option code */
enum
{
	SSE_LSP_UNDEFSYMBOL = (1 << 0)
};

/* error code */
enum 
{
	SSE_LSP_ENOERR,
	SSE_LSP_ENOMEM,

	SSE_LSP_ERR_ABORT,
	SSE_LSP_ERR_END,
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

	SSE_LSP_EDIVBYZERO
};

typedef sse_lsp_obj_t* (*sse_lsp_prim_t) (sse_lsp_t* lsp, sse_lsp_obj_t* obj);

#ifdef __cplusplus
extern "C" {
#endif

sse_lsp_t* sse_lsp_open (
	const sse_lsp_syscas_t* syscas,
	sse_size_t mem_ubound, sse_size_t mem_ubound_inc);

void sse_lsp_close (sse_lsp_t* lsp);

int sse_lsp_geterrnum (sse_lsp_t* lsp);

int sse_lsp_attach_input (sse_lsp_t* lsp, sse_lsp_io_t input, void* arg);
int sse_lsp_detach_input (sse_lsp_t* lsp);

int sse_lsp_attach_output (sse_lsp_t* lsp, sse_lsp_io_t output, void* arg);
int sse_lsp_detach_output (sse_lsp_t* lsp);

sse_lsp_obj_t* sse_lsp_read (sse_lsp_t* lsp);
sse_lsp_obj_t* sse_lsp_eval (sse_lsp_t* lsp, sse_lsp_obj_t* obj);
int sse_lsp_print (sse_lsp_t* lsp, const sse_lsp_obj_t* obj);

int sse_lsp_add_prim (sse_lsp_t* lsp, const sse_char_t* name, sse_lsp_prim_t prim);
int sse_lsp_remove_prim (sse_lsp_t* lsp, const sse_char_t* name);


const sse_char_t* sse_lsp_geterrstr (int errnum);

#ifdef __cplusplus
}
#endif

#endif
