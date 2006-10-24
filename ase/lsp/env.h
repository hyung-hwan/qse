/*
 * $Id: env.h,v 1.9 2006-10-24 04:22:39 bacon Exp $
 */

#ifndef _ASE_LSP_ENV_H_
#define _ASE_LSP_ENV_H_

#include <ase/lsp/obj.h>

struct ase_lsp_assoc_t
{
	ase_lsp_obj_t* name; // ase_lsp_obj_symbol_t
	/*ase_lsp_obj_t* value;*/
	ase_lsp_obj_t* value; /* value as a variable */
	ase_lsp_obj_t* func;  /* function definition */
	struct ase_lsp_assoc_t* link;
};

struct ase_lsp_frame_t
{
	struct ase_lsp_assoc_t* assoc;
	struct ase_lsp_frame_t* link;
};

typedef struct ase_lsp_assoc_t ase_lsp_assoc_t;
typedef struct ase_lsp_frame_t ase_lsp_frame_t;

#ifdef __cplusplus
extern "C" {
#endif

ase_lsp_assoc_t* ase_lsp_assoc_new (
	ase_lsp_obj_t* name, ase_lsp_obj_t* value, ase_lsp_obj_t* func);
void ase_lsp_assoc_free (ase_lsp_assoc_t* assoc);

ase_lsp_frame_t* ase_lsp_frame_new (void);
void ase_lsp_frame_free (ase_lsp_frame_t* frame);
ase_lsp_assoc_t* ase_lsp_frame_lookup (ase_lsp_frame_t* frame, ase_lsp_obj_t* name);

ase_lsp_assoc_t* ase_lsp_frame_insert_value (
	ase_lsp_frame_t* frame, ase_lsp_obj_t* name, ase_lsp_obj_t* value);
ase_lsp_assoc_t* ase_lsp_frame_insert_func (
	ase_lsp_frame_t* frame, ase_lsp_obj_t* name, ase_lsp_obj_t* func);

#ifdef __cplusplus
}
#endif

#endif
