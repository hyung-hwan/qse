/*
 * $Id: env.c,v 1.9 2006-10-22 13:10:45 bacon Exp $
 */

#include <sse/lsp/env.h>
#include <sse/bas/memory.h>
#include <sse/bas/assert.h>

// TODO: make the frame hash accessible....

sse_lsp_assoc_t* sse_lsp_assoc_new (
	sse_lsp_obj_t* name, sse_lsp_obj_t* value, sse_lsp_obj_t* func)
{
	sse_lsp_assoc_t* assoc;

	assoc = (sse_lsp_assoc_t*) sse_malloc (sizeof(sse_lsp_assoc_t));
	if (assoc == SSE_NULL) return SSE_NULL;

	assoc->name  = name;
	assoc->value = value;
	assoc->func  = func;
	assoc->link  = SSE_NULL;

	return assoc;
}

void sse_lsp_assoc_free (sse_lsp_assoc_t* assoc)
{
	sse_free (assoc);
}

sse_lsp_frame_t* sse_lsp_frame_new (void)
{
	sse_lsp_frame_t* frame;

	frame = (sse_lsp_frame_t*) sse_malloc (sizeof(sse_lsp_frame_t));
	if (frame == SSE_NULL) return SSE_NULL;

	frame->assoc = SSE_NULL;
	frame->link  = SSE_NULL;

	return frame;
}

void sse_lsp_frame_free (sse_lsp_frame_t* frame)
{
	sse_lsp_assoc_t* assoc, * link;

	// destroy the associations
	assoc = frame->assoc;
	while (assoc != SSE_NULL) {
		link = assoc->link;
		sse_lsp_assoc_free (assoc);	
		assoc = link;
	}

	sse_free (frame);
}

sse_lsp_assoc_t* sse_lsp_frame_lookup (sse_lsp_frame_t* frame, sse_lsp_obj_t* name)
{
	sse_lsp_assoc_t* assoc;

	sse_assert (SSE_LSP_TYPE(name) == SSE_LSP_OBJ_SYMBOL);

	assoc = frame->assoc;
	while (assoc != SSE_NULL) {
		if (name == assoc->name) return assoc;
		assoc = assoc->link;
	}
	return SSE_NULL;
}

sse_lsp_assoc_t* sse_lsp_frame_insert_value (
	sse_lsp_frame_t* frame, sse_lsp_obj_t* name, sse_lsp_obj_t* value)
{
	sse_lsp_assoc_t* assoc;

	sse_assert (SSE_LSP_TYPE(name) == SSE_LSP_OBJ_SYMBOL);

	assoc = sse_lsp_assoc_new (name, value, SSE_NULL);
	if (assoc == SSE_NULL) return SSE_NULL;
	assoc->link  = frame->assoc;
	frame->assoc = assoc;
	return assoc;
}

sse_lsp_assoc_t* sse_lsp_frame_insert_func (
	sse_lsp_frame_t* frame, sse_lsp_obj_t* name, sse_lsp_obj_t* func)
{
	sse_lsp_assoc_t* assoc;

	sse_assert (SSE_LSP_TYPE(name) == SSE_LSP_OBJ_SYMBOL);

	assoc = sse_lsp_assoc_new (name, SSE_NULL, func);
	if (assoc == SSE_NULL) return SSE_NULL;
	assoc->link  = frame->assoc;
	frame->assoc = assoc;
	return assoc;
}
