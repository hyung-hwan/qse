/*
 * $Id: env.c,v 1.11 2006-10-25 13:42:30 bacon Exp $
 */

#include <ase/lsp/lsp_i.h>

// TODO: make the frame hash accessible....

ase_lsp_assoc_t* ase_lsp_assoc_new (
	ase_lsp_obj_t* name, ase_lsp_obj_t* value, ase_lsp_obj_t* func)
{
	ase_lsp_assoc_t* assoc;

	assoc = (ase_lsp_assoc_t*) ase_malloc (sizeof(ase_lsp_assoc_t));
	if (assoc == ASE_NULL) return ASE_NULL;

	assoc->name  = name;
	assoc->value = value;
	assoc->func  = func;
	assoc->link  = ASE_NULL;

	return assoc;
}

void ase_lsp_assoc_free (ase_lsp_assoc_t* assoc)
{
	ase_free (assoc);
}

ase_lsp_frame_t* ase_lsp_frame_new (void)
{
	ase_lsp_frame_t* frame;

	frame = (ase_lsp_frame_t*) ase_malloc (sizeof(ase_lsp_frame_t));
	if (frame == ASE_NULL) return ASE_NULL;

	frame->assoc = ASE_NULL;
	frame->link  = ASE_NULL;

	return frame;
}

void ase_lsp_frame_free (ase_lsp_frame_t* frame)
{
	ase_lsp_assoc_t* assoc, * link;

	// destroy the associations
	assoc = frame->assoc;
	while (assoc != ASE_NULL) 
	{
		link = assoc->link;
		ase_lsp_assoc_free (assoc);	
		assoc = link;
	}

	ase_free (frame);
}

ase_lsp_assoc_t* ase_lsp_frame_lookup (ase_lsp_frame_t* frame, ase_lsp_obj_t* name)
{
	ase_lsp_assoc_t* assoc;

	ase_assert (ASE_LSP_TYPE(name) == ASE_LSP_OBJ_SYM);

	assoc = frame->assoc;
	while (assoc != ASE_NULL) 
	{
		if (name == assoc->name) return assoc;
		assoc = assoc->link;
	}
	return ASE_NULL;
}

ase_lsp_assoc_t* ase_lsp_frame_insert_value (
	ase_lsp_frame_t* frame, ase_lsp_obj_t* name, ase_lsp_obj_t* value)
{
	ase_lsp_assoc_t* assoc;

	ase_assert (ASE_LSP_TYPE(name) == ASE_LSP_OBJ_SYM);

	assoc = ase_lsp_assoc_new (name, value, ASE_NULL);
	if (assoc == ASE_NULL) return ASE_NULL;
	assoc->link  = frame->assoc;
	frame->assoc = assoc;
	return assoc;
}

ase_lsp_assoc_t* ase_lsp_frame_insert_func (
	ase_lsp_frame_t* frame, ase_lsp_obj_t* name, ase_lsp_obj_t* func)
{
	ase_lsp_assoc_t* assoc;

	ase_assert (ASE_LSP_TYPE(name) == ASE_LSP_OBJ_SYM);

	assoc = ase_lsp_assoc_new (name, ASE_NULL, func);
	if (assoc == ASE_NULL) return ASE_NULL;
	assoc->link  = frame->assoc;
	frame->assoc = assoc;
	return assoc;
}
