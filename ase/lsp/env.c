/*
 * $Id: env.c,v 1.17 2007-03-06 14:58:00 bacon Exp $
 *
 * {License}
 */

#include <ase/lsp/lsp_i.h>

/* TODO: make the frame hash accessible */

static ase_lsp_assoc_t* __new_assoc (
	ase_lsp_t* lsp, ase_lsp_obj_t* name, 
	ase_lsp_obj_t* value, ase_lsp_obj_t* func)
{
	ase_lsp_assoc_t* assoc;

	assoc = (ase_lsp_assoc_t*) 
		ASE_LSP_MALLOC (lsp, sizeof(ase_lsp_assoc_t));
	if (assoc == ASE_NULL) 
	{
		ase_lsp_seterror (lsp, ASE_LSP_ENOMEM, ASE_NULL, 0);
		return ASE_NULL;
	}

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
	if (frame == ASE_NULL) 
	{
		ase_lsp_seterror (lsp, ASE_LSP_ENOMEM, ASE_NULL, 0);
		return ASE_NULL;
	}

	frame->assoc = ASE_NULL;
	frame->link  = ASE_NULL;

	return frame;
}

void ase_lsp_freeframe (ase_lsp_t* lsp, ase_lsp_frame_t* frame)
{
	ase_lsp_assoc_t* assoc, * link;

	/* destroy the associations */
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

	ASE_ASSERT (ASE_LSP_TYPE(name) == ASE_LSP_OBJ_SYM);

	assoc = frame->assoc;
	while (assoc != ASE_NULL) 
	{
		if (name == assoc->name) return assoc;
		assoc = assoc->link;
	}
	return ASE_NULL;
}

ase_lsp_assoc_t* ase_lsp_insvalueintoframe (
	ase_lsp_t* lsp, ase_lsp_frame_t* frame, 
	ase_lsp_obj_t* name, ase_lsp_obj_t* value)
{
	ase_lsp_assoc_t* assoc;

	ASE_ASSERT (ASE_LSP_TYPE(name) == ASE_LSP_OBJ_SYM);

	assoc = __new_assoc (lsp, name, value, ASE_NULL);
	if (assoc == ASE_NULL) return ASE_NULL;

	assoc->link  = frame->assoc;
	frame->assoc = assoc;
	return assoc;
}

ase_lsp_assoc_t* ase_lsp_insfuncintoframe (
	ase_lsp_t* lsp, ase_lsp_frame_t* frame, 
	ase_lsp_obj_t* name, ase_lsp_obj_t* func)
{
	ase_lsp_assoc_t* assoc;

	ASE_ASSERT (ASE_LSP_TYPE(name) == ASE_LSP_OBJ_SYM);

	assoc = __new_assoc (lsp, name, ASE_NULL, func);
	if (assoc == ASE_NULL) return ASE_NULL;

	assoc->link  = frame->assoc;
	frame->assoc = assoc;
	return assoc;
}

ase_lsp_tlink_t* ase_lsp_pushtmp (ase_lsp_t* lsp, ase_lsp_obj_t* obj)
{
	ase_lsp_tlink_t* tlink;

	tlink = (ase_lsp_tlink_t*) 
		ASE_LSP_MALLOC (lsp, sizeof(ase_lsp_tlink_t));
	if (tlink == ASE_NULL) 
	{
		ase_lsp_seterror (lsp, ASE_LSP_ENOMEM, ASE_NULL, 0);
		return ASE_NULL;
	}

	tlink->obj = obj;
	tlink->link = lsp->mem->tlink;
	lsp->mem->tlink = tlink;
	lsp->mem->tlink_count++;

	return tlink;
}

void ase_lsp_poptmp (ase_lsp_t* lsp)
{
	ase_lsp_tlink_t* top;

	ASE_ASSERT (lsp->mem->tlink != ASE_NULL);

	top = lsp->mem->tlink;
	lsp->mem->tlink = top->link;
	lsp->mem->tlink_count--;

	ASE_LSP_FREE (lsp, top);
}
