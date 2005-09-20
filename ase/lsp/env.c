/*
 * $Id: env.c,v 1.8 2005-09-20 09:17:06 bacon Exp $
 */

#include <xp/lsp/env.h>
#include <xp/bas/memory.h>
#include <xp/bas/assert.h>

// TODO: make the frame hash accessible....

xp_lsp_assoc_t* xp_lsp_assoc_new (
	xp_lsp_obj_t* name, xp_lsp_obj_t* value, xp_lsp_obj_t* func)
{
	xp_lsp_assoc_t* assoc;

	assoc = (xp_lsp_assoc_t*) xp_malloc (sizeof(xp_lsp_assoc_t));
	if (assoc == XP_NULL) return XP_NULL;

	assoc->name  = name;
	assoc->value = value;
	assoc->func  = func;
	assoc->link  = XP_NULL;

	return assoc;
}

void xp_lsp_assoc_free (xp_lsp_assoc_t* assoc)
{
	xp_free (assoc);
}

xp_lsp_frame_t* xp_lsp_frame_new (void)
{
	xp_lsp_frame_t* frame;

	frame = (xp_lsp_frame_t*) xp_malloc (sizeof(xp_lsp_frame_t));
	if (frame == XP_NULL) return XP_NULL;

	frame->assoc = XP_NULL;
	frame->link  = XP_NULL;

	return frame;
}

void xp_lsp_frame_free (xp_lsp_frame_t* frame)
{
	xp_lsp_assoc_t* assoc, * link;

	// destroy the associations
	assoc = frame->assoc;
	while (assoc != XP_NULL) {
		link = assoc->link;
		xp_lsp_assoc_free (assoc);	
		assoc = link;
	}

	xp_free (frame);
}

xp_lsp_assoc_t* xp_lsp_frame_lookup (xp_lsp_frame_t* frame, xp_lsp_obj_t* name)
{
	xp_lsp_assoc_t* assoc;

	xp_assert (XP_LSP_TYPE(name) == XP_LSP_OBJ_SYMBOL);

	assoc = frame->assoc;
	while (assoc != XP_NULL) {
		if (name == assoc->name) return assoc;
		assoc = assoc->link;
	}
	return XP_NULL;
}

xp_lsp_assoc_t* xp_lsp_frame_insert_value (
	xp_lsp_frame_t* frame, xp_lsp_obj_t* name, xp_lsp_obj_t* value)
{
	xp_lsp_assoc_t* assoc;

	xp_assert (XP_LSP_TYPE(name) == XP_LSP_OBJ_SYMBOL);

	assoc = xp_lsp_assoc_new (name, value, XP_NULL);
	if (assoc == XP_NULL) return XP_NULL;
	assoc->link  = frame->assoc;
	frame->assoc = assoc;
	return assoc;
}

xp_lsp_assoc_t* xp_lsp_frame_insert_func (
	xp_lsp_frame_t* frame, xp_lsp_obj_t* name, xp_lsp_obj_t* func)
{
	xp_lsp_assoc_t* assoc;

	xp_assert (XP_LSP_TYPE(name) == XP_LSP_OBJ_SYMBOL);

	assoc = xp_lsp_assoc_new (name, XP_NULL, func);
	if (assoc == XP_NULL) return XP_NULL;
	assoc->link  = frame->assoc;
	frame->assoc = assoc;
	return assoc;
}
