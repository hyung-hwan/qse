/*
 * $Id: env.c,v 1.14 2007-02-03 10:51:52 bacon Exp $
 *
 * {License}
 */

#include <ase/lsp/lsp_i.h>

// TODO: make the frame hash accessible....

static ase_lsp_assoc_t* __new_assoc (
	ase_lsp_t* lsp, ase_lsp_obj_t* name, 
	ase_lsp_obj_t* value, ase_lsp_obj_t* func)
{
	ase_lsp_assoc_t* assoc;

	assoc = (ase_lsp_assoc_t*) 
		ASE_LSP_MALLOC (lsp, sizeof(ase_lsp_assoc_t));
	if (assoc == ASE_NULL) return ASE_NULL;

	assoc->name  = name;
	assoc->value = value;
	assoc->func  = func;
	assoc->link  = ASE_NULL;

	return assoc;
}

ase_lsp_frame_t* ase_lsp_newframe (ase_lsp_t* lsp)
{
	ase_lsp_frame_t* frame;

	frame = (ase_lsp_frame_t*) 
		ASE_LSP_MALLOC (lsp, sizeof(ase_lsp_frame_t));
	if (frame == ASE_NULL) return ASE_NULL;

	frame->assoc = ASE_NULL;
	frame->link  = ASE_NULL;

	return frame;
}

void ase_lsp_freeframe (ase_lsp_t* lsp, ase_lsp_frame_t* frame)
{
	ase_lsp_assoc_t* assoc, * link;

	// destroy the associations
	assoc = frame->assoc;
	while (assoc != ASE_NULL) 
	{
		link = assoc->link;
		ASE_LSP_FREE (lsp, assoc);
		assoc = link;
	}

	ASE_LSP_FREE (lsp, frame);
}

ase_lsp_assoc_t* ase_lsp_lookupinframe (
	ase_lsp_t* lsp, ase_lsp_frame_t* frame, ase_lsp_obj_t* name)
{
	ase_lsp_assoc_t* assoc;

	ASE_LSP_ASSERT (lsp, ASE_LSP_TYPE(name) == ASE_LSP_OBJ_SYM);

	assoc = frame->assoc;
	while (assoc != ASE_NULL) 
	{
		if (name == assoc->name) return assoc;
		assoc = assoc->link;
	}
	return ASE_NULL;
}

ase_lsp_assoc_t* ase_lsp_insertvalueintoframe (
	ase_lsp_t* lsp, ase_lsp_frame_t* frame, 
	ase_lsp_obj_t* name, ase_lsp_obj_t* value)
{
	ase_lsp_assoc_t* assoc;

	ASE_LSP_ASSERT (lsp, ASE_LSP_TYPE(name) == ASE_LSP_OBJ_SYM);

	assoc = __new_assoc (lsp, name, value, ASE_NULL);
	if (assoc == ASE_NULL) return ASE_NULL;
	assoc->link  = frame->assoc;
	frame->assoc = assoc;
	return assoc;
}

ase_lsp_assoc_t* ase_lsp_insertfuncintoframe (
	ase_lsp_t* lsp, ase_lsp_frame_t* frame, 
	ase_lsp_obj_t* name, ase_lsp_obj_t* func)
{
	ase_lsp_assoc_t* assoc;

	ASE_LSP_ASSERT (lsp, ASE_LSP_TYPE(name) == ASE_LSP_OBJ_SYM);

	assoc = __new_assoc (lsp, name, ASE_NULL, func);
	if (assoc == ASE_NULL) return ASE_NULL;
	assoc->link  = frame->assoc;
	frame->assoc = assoc;
	return assoc;
}
