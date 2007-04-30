/*
 * $Id: env.h,v 1.1.1.1 2007/03/28 14:05:24 bacon Exp $
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
typedef struct ase_lsp_tlink_t ase_lsp_tlink_t;

struct ase_lsp_assoc_t
{
	ase_lsp_obj_t* name;  /* ase_lsp_obj_sym_t */
	ase_lsp_obj_t* value; /* value as a variable */
	ase_lsp_obj_t* func;  /* function definition */

	ase_lsp_assoc_t* link;
};

struct ase_lsp_frame_t
{
	ase_lsp_assoc_t* assoc;
	ase_lsp_frame_t* link;
};

struct ase_lsp_tlink_t
{
	ase_lsp_obj_t*   obj;
	ase_lsp_tlink_t* link;
};

#ifdef __cplusplus
extern "C" {
#endif

ase_lsp_frame_t* ase_lsp_newframe (ase_lsp_t* lsp);
void ase_lsp_freeframe (ase_lsp_t* lsp, ase_lsp_frame_t* frame);

ase_lsp_assoc_t* ase_lsp_lookupinframe (
	ase_lsp_t* lsp, ase_lsp_frame_t* frame, ase_lsp_obj_t* name);

ase_lsp_assoc_t* ase_lsp_insvalueintoframe (
	ase_lsp_t* lsp, ase_lsp_frame_t* frame, 
	ase_lsp_obj_t* name, ase_lsp_obj_t* value);
ase_lsp_assoc_t* ase_lsp_insfuncintoframe (
	ase_lsp_t* lsp, ase_lsp_frame_t* frame, 
	ase_lsp_obj_t* name, ase_lsp_obj_t* func);

ase_lsp_tlink_t* ase_lsp_pushtmp (ase_lsp_t* lsp, ase_lsp_obj_t* obj);
void ase_lsp_poptmp (ase_lsp_t* lsp);

#ifdef __cplusplus
}
#endif

#endif
