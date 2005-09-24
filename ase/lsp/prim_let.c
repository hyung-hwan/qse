/*
 * $Id: prim_let.c,v 1.3 2005-09-24 08:16:02 bacon Exp $
 */

#include <xp/lsp/prim.h>

static xp_lsp_obj_t* __prim_let (
	xp_lsp_t* lsp, xp_lsp_obj_t* args, int sequential)
{
	xp_lsp_frame_t* frame;
	xp_lsp_obj_t* assoc;
	xp_lsp_obj_t* body;
	xp_lsp_obj_t* value;

	XP_LSP_PRIM_CHECK_ARG_COUNT (lsp, args, 1, XP_LSP_PRIM_MAX_ARG_COUNT);

	// create a new frame
	frame = xp_lsp_frame_new ();
	if (frame == XP_NULL) {
		lsp->errnum = XP_LSP_ERR_MEMORY;
		return XP_NULL;
	}
	//frame->link = lsp->mem->frame;

	if (sequential) {
		frame->link = lsp->mem->frame;
		lsp->mem->frame = frame;
	}
	else {
		frame->link = lsp->mem->brooding_frame;
		lsp->mem->brooding_frame = frame;
	}

	assoc = XP_LSP_CAR(args);

	//while (assoc != lsp->mem->nil) {
	while (XP_LSP_TYPE(assoc) == XP_LSP_OBJ_CONS) {
		xp_lsp_obj_t* ass = XP_LSP_CAR(assoc);
		if (XP_LSP_TYPE(ass) == XP_LSP_OBJ_CONS) {
			xp_lsp_obj_t* n = XP_LSP_CAR(ass);
			xp_lsp_obj_t* v = XP_LSP_CDR(ass);

			if (XP_LSP_TYPE(n) != XP_LSP_OBJ_SYMBOL) {
				lsp->errnum = XP_LSP_ERR_BAD_ARG; // must be a symbol
				if (sequential) lsp->mem->frame = frame->link;
				else lsp->mem->brooding_frame = frame->link;
				xp_lsp_frame_free (frame);
				return XP_NULL;
			}

			if (v != lsp->mem->nil) {
				if (XP_LSP_CDR(v) != lsp->mem->nil) {
					lsp->errnum = XP_LSP_ERR_TOO_MANY_ARGS; // must be a symbol
					if (sequential) lsp->mem->frame = frame->link;
					else lsp->mem->brooding_frame = frame->link;
					xp_lsp_frame_free (frame);
					return XP_NULL;
				}
				if ((v = xp_lsp_eval(lsp, XP_LSP_CAR(v))) == XP_NULL) {
					if (sequential) lsp->mem->frame = frame->link;
					else lsp->mem->brooding_frame = frame->link;
					xp_lsp_frame_free (frame);
					return XP_NULL;
				}
			}

			if (xp_lsp_frame_lookup (frame, n) != XP_NULL) {
				lsp->errnum = XP_LSP_ERR_DUP_FORMAL;
				if (sequential) lsp->mem->frame = frame->link;
				else lsp->mem->brooding_frame = frame->link;
				xp_lsp_frame_free (frame);
				return XP_NULL;
			}
			if (xp_lsp_frame_insert_value(frame, n, v) == XP_NULL) {
				lsp->errnum = XP_LSP_ERR_MEMORY;
				if (sequential) lsp->mem->frame = frame->link;
				else lsp->mem->brooding_frame = frame->link;
				xp_lsp_frame_free (frame);
				return XP_NULL;
			}
		}
		else if (XP_LSP_TYPE(ass) == XP_LSP_OBJ_SYMBOL) {
			if (xp_lsp_frame_lookup(frame, ass) != XP_NULL) {
				lsp->errnum = XP_LSP_ERR_DUP_FORMAL;
				if (sequential) lsp->mem->frame = frame->link;
				else lsp->mem->brooding_frame = frame->link;
				xp_lsp_frame_free (frame);
				return XP_NULL;
			}
			if (xp_lsp_frame_insert_value(frame, ass, lsp->mem->nil) == XP_NULL) {
				lsp->errnum = XP_LSP_ERR_MEMORY;
				if (sequential) lsp->mem->frame = frame->link;
				else lsp->mem->brooding_frame = frame->link;
				xp_lsp_frame_free (frame);
				return XP_NULL;
			}
		}
		else {
			lsp->errnum = XP_LSP_ERR_BAD_ARG;		
			if (sequential) lsp->mem->frame = frame->link;
			else lsp->mem->brooding_frame = frame->link;
			xp_lsp_frame_free (frame);
			return XP_NULL;
		}

		assoc = XP_LSP_CDR(assoc);
	}

	if (assoc != lsp->mem->nil) {
		lsp->errnum = XP_LSP_ERR_BAD_ARG;	
		if (sequential) lsp->mem->frame = frame->link;
		else lsp->mem->brooding_frame = frame->link;
		xp_lsp_frame_free (frame);
		return XP_NULL;
	}

	// push the frame
	if (!sequential) {
		lsp->mem->brooding_frame = frame->link;
		frame->link = lsp->mem->frame;
		lsp->mem->frame = frame;
	}

	// evaluate forms in the body
	value = lsp->mem->nil;
	body = XP_LSP_CDR(args);
	while (body != lsp->mem->nil) {
		value = xp_lsp_eval (lsp, XP_LSP_CAR(body));
		if (value == XP_NULL) {
			lsp->mem->frame = frame->link;
			xp_lsp_frame_free (frame);
			return XP_NULL;
		}
		body = XP_LSP_CDR(body);
	}

	// pop the frame
	lsp->mem->frame = frame->link;

	// destroy the frame
	xp_lsp_frame_free (frame);
	return value;
}

xp_lsp_obj_t* xp_lsp_prim_let (xp_lsp_t* lsp, xp_lsp_obj_t* args)
{
	/*
	 * (defun x (x y) 
	 *     (let ((temp1 10) (temp2 20)) 
	 *          (+ x y temp1 temp2)))
	 * (x 40 50)
	 * temp1 
	 */
	return __prim_let (lsp, args, 0);
}

xp_lsp_obj_t* xp_lsp_prim_letx (xp_lsp_t* lsp, xp_lsp_obj_t* args)
{
	return __prim_let (lsp, args, 1);
}
