/*
 * $Id: lsp.h,v 1.25 2006-10-24 04:22:39 bacon Exp $
 */

#ifndef _ASE_LSP_LSP_H_
#define _ASE_LSP_LSP_H_

#include <ase/lsp/types.h>

typedef struct ase_lsp_t ase_lsp_t;
typedef struct ase_lsp_obj_t ase_lsp_obj_t;
typedef struct ase_lsp_syscas_t ase_lsp_syscas_t;

typedef ase_ssize_t (*ase_lsp_io_t) (
	int cmd, void* arg, ase_char_t* data, ase_size_t count);

struct ase_lsp_syscas_t
{
	/* memory */
	void* (*malloc) (ase_size_t n, void* custom_data);
	void* (*realloc) (void* ptr, ase_size_t n, void* custom_data);
	void  (*free) (void* ptr, void* custom_data);

	/* character class */
	ase_bool_t (*is_upper)  (ase_cint_t c);
	ase_bool_t (*is_lower)  (ase_cint_t c);
	ase_bool_t (*is_alpha)  (ase_cint_t c);
	ase_bool_t (*is_digit)  (ase_cint_t c);
	ase_bool_t (*is_xdigit) (ase_cint_t c);
	ase_bool_t (*is_alnum)  (ase_cint_t c);
	ase_bool_t (*is_space)  (ase_cint_t c);
	ase_bool_t (*is_print)  (ase_cint_t c);
	ase_bool_t (*is_graph)  (ase_cint_t c);
	ase_bool_t (*is_cntrl)  (ase_cint_t c);
	ase_bool_t (*is_punct)  (ase_cint_t c);
	ase_cint_t (*to_upper)  (ase_cint_t c);
	ase_cint_t (*to_lower)  (ase_cint_t c);

	/* utilities */
	void* (*memcpy)  (void* dst, const void* src, ase_size_t n);
	void* (*memset)  (void* dst, int val, ase_size_t n);

	int (*sprintf) (ase_char_t* buf, ase_size_t size, ase_char_t* fmt, ...);
	int (*dprintf) (ase_char_t* fmt, ...);
	void (*abort) (void);

	void* custom_data;
};

/* io function commands */
enum 
{
	ASE_LSP_IO_OPEN   = 0,
	ASE_LSP_IO_CLOSE  = 1,
	ASE_LSP_IO_READ   = 2,
	ASE_LSP_IO_WRITE  = 3
};

/* option code */
enum
{
	ASE_LSP_UNDEFSYMBOL = (1 << 0)
};

/* error code */
enum 
{
	ASE_LSP_ENOERR,
	ASE_LSP_ENOMEM,

	ASE_LSP_ERR_ABORT,
	ASE_LSP_ERR_END,
	ASE_LSP_ERR_INPUT_NOT_ATTACHED,
	ASE_LSP_ERR_INPUT,
	ASE_LSP_ERR_OUTPUT_NOT_ATTACHED,
	ASE_LSP_ERR_OUTPUT,
	ASE_LSP_ERR_SYNTAX,
	ASE_LSP_ERR_BAD_ARG,
	ASE_LSP_ERR_WRONG_ARG,
	ASE_LSP_ERR_TOO_FEW_ARGS,
	ASE_LSP_ERR_TOO_MANY_ARGS,
	ASE_LSP_ERR_UNDEF_FUNC,
	ASE_LSP_ERR_BAD_FUNC,
	ASE_LSP_ERR_DUP_FORMAL,
	ASE_LSP_ERR_BAD_SYMBOL,
	ASE_LSP_ERR_UNDEF_SYMBOL,
	ASE_LSP_ERR_EMPTY_BODY,
	ASE_LSP_ERR_BAD_VALUE,

	ASE_LSP_EDIVBYZERO
};

typedef ase_lsp_obj_t* (*ase_lsp_prim_t) (ase_lsp_t* lsp, ase_lsp_obj_t* obj);

#ifdef __cplusplus
extern "C" {
#endif

ase_lsp_t* ase_lsp_open (
	const ase_lsp_syscas_t* syscas,
	ase_size_t mem_ubound, ase_size_t mem_ubound_inc);

void ase_lsp_close (ase_lsp_t* lsp);

int ase_lsp_geterrnum (ase_lsp_t* lsp);

int ase_lsp_attach_input (ase_lsp_t* lsp, ase_lsp_io_t input, void* arg);
int ase_lsp_detach_input (ase_lsp_t* lsp);

int ase_lsp_attach_output (ase_lsp_t* lsp, ase_lsp_io_t output, void* arg);
int ase_lsp_detach_output (ase_lsp_t* lsp);

ase_lsp_obj_t* ase_lsp_read (ase_lsp_t* lsp);
ase_lsp_obj_t* ase_lsp_eval (ase_lsp_t* lsp, ase_lsp_obj_t* obj);
int ase_lsp_print (ase_lsp_t* lsp, const ase_lsp_obj_t* obj);

int ase_lsp_add_prim (ase_lsp_t* lsp, const ase_char_t* name, ase_lsp_prim_t prim);
int ase_lsp_remove_prim (ase_lsp_t* lsp, const ase_char_t* name);


const ase_char_t* ase_lsp_geterrstr (int errnum);

#ifdef __cplusplus
}
#endif

#endif
