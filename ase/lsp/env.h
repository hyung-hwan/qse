/*
 * $Id: env.h,v 1.11 2007-02-03 10:51:52 bacon Exp $
 *
 * {License}
 */

#ifndef _ASE_LSP_ENV_H_
#define _ASE_LSP_ENV_H_

#ifndef _ASE_LSP_LSP_H_
#error Never include this file directly. Include <ase/lsp/lsp.h> instead
#endif

typedef struct ase_lsp_assoc_t ase_lsp_assoc_t;
typedef struct ase_lsp_frame_t ase_lsp_frame_t;

struct ase_lsp_assoc_t
{
	ase_lsp_obj_t* name; /* ase_lsp_obj_symbol_t */
	/*ase_lsp_obj_t* value;*/
	ase_lsp_obj_t* value; /* value as a variable */
	ase_lsp_obj_t* func;  /* function definition */

	ase_lsp_assoc_t* link;
};

struct ase_lsp_frame_t
{
	ase_lsp_assoc_t* assoc;
	ase_lsp_frame_t* link;
};

#ifdef __cplusplus
extern "C" {
#endif

ase_lsp_frame_t* ase_lsp_newframe (ase_lsp_t* lsp);
void ase_lsp_freeframe (ase_lsp_t* lsp, ase_lsp_frame_t* frame);

ase_lsp_assoc_t* ase_lsp_lookupinframe (
	ase_lsp_t* lsp, ase_lsp_frame_t* frame, ase_lsp_obj_t* name);

ase_lsp_assoc_t* ase_lsp_insertvalueintoframe (
	ase_lsp_t* lsp, ase_lsp_frame_t* frame, 
	ase_lsp_obj_t* name, ase_lsp_obj_t* value);
ase_lsp_assoc_t* ase_lsp_insertfuncintoframe (
	ase_lsp_t* lsp, ase_lsp_frame_t* frame, 
	ase_lsp_obj_t* name, ase_lsp_obj_t* func);

#ifdef __cplusplus
}
#endif

#endif
