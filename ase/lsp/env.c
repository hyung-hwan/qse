/*
 * $Id: env.c,v 1.2 2005-02-04 16:00:37 bacon Exp $
 */

#include <xp/lisp/env.h>
#include <xp/c/stdlib.h>

xp_lisp_assoc_t* xp_lisp_assoc_new (xp_lisp_obj_t* name, xp_lisp_obj_t* value)
{
	xp_lisp_assoc_t* assoc;

	assoc = (xp_lisp_assoc_t*) xp_malloc (sizeof(xp_lisp_assoc_t));
	if (assoc == XP_NULL) return XP_NULL;

	assoc->name  = name;
	assoc->value = value;
	assoc->link  = XP_NULL;

	return assoc;
}

void xp_lisp_assoc_free (xp_lisp_assoc_t* assoc)
{
	xp_free (assoc);
}

xp_lisp_frame_t* xp_lisp_frame_new (void)
{
	xp_lisp_frame_t* frame;

	frame = (xp_lisp_frame_t*) xp_malloc (sizeof(xp_lisp_frame_t));
	if (frame == XP_NULL) return XP_NULL;

	frame->assoc = XP_NULL;
	frame->link  = XP_NULL;

	return frame;
}

void xp_lisp_frame_free (xp_lisp_frame_t* frame)
{
	xp_lisp_assoc_t* assoc, * link;

	// destroy the associations
	assoc = frame->assoc;
	while (assoc != XP_NULL) {
		link = assoc->link;
		xp_lisp_assoc_free (assoc);	
		assoc = link;
	}

	xp_free (frame);
}

xp_lisp_assoc_t* xp_lisp_frame_lookup (xp_lisp_frame_t* frame, xp_lisp_obj_t* name)
{
	xp_lisp_assoc_t* assoc;

	xp_lisp_assert (XP_LISP_TYPE(name) == XP_LISP_OBJ_SYMBOL);

	assoc = frame->assoc;
	while (assoc != XP_NULL) {
		if (name == assoc->name) return assoc;
		assoc = assoc->link;
	}
	return XP_NULL;
}

xp_lisp_assoc_t* xp_lisp_frame_insert (
	xp_lisp_frame_t* frame, xp_lisp_obj_t* name, xp_lisp_obj_t* value)
{
	xp_lisp_assoc_t* assoc;

	xp_lisp_assert (XP_LISP_TYPE(name) == XP_LISP_OBJ_SYMBOL);

	assoc = xp_lisp_assoc_new (name, value);
	if (assoc == XP_NULL) return XP_NULL;
	assoc->link  = frame->assoc;
	frame->assoc = assoc;
	return assoc;
}

