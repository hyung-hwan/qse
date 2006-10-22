/*
 * $Id: prim_let.c,v 1.4 2006-10-22 13:10:46 bacon Exp $
 */

#include <sse/lsp/prim.h>

static sse_lsp_obj_t* __prim_let (
	sse_lsp_t* lsp, sse_lsp_obj_t* args, int sequential)
{
	sse_lsp_frame_t* frame;
	sse_lsp_obj_t* assoc;
	sse_lsp_obj_t* body;
	sse_lsp_obj_t* value;

	SSE_LSP_PRIM_CHECK_ARG_COUNT (lsp, args, 1, SSE_LSP_PRIM_MAX_ARG_COUNT);

	// create a new frame
	frame = sse_lsp_frame_new ();
	if (frame == SSE_NULL) {
		lsp->errnum = SSE_LSP_ERR_MEMORY;
		return SSE_NULL;
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

	assoc = SSE_LSP_CAR(args);

	//while (assoc != lsp->mem->nil) {
	while (SSE_LSP_TYPE(assoc) == SSE_LSP_OBJ_CONS) {
		sse_lsp_obj_t* ass = SSE_LSP_CAR(assoc);
		if (SSE_LSP_TYPE(ass) == SSE_LSP_OBJ_CONS) {
			sse_lsp_obj_t* n = SSE_LSP_CAR(ass);
			sse_lsp_obj_t* v = SSE_LSP_CDR(ass);

			if (SSE_LSP_TYPE(n) != SSE_LSP_OBJ_SYMBOL) {
				lsp->errnum = SSE_LSP_ERR_BAD_ARG; // must be a symbol
				if (sequential) lsp->mem->frame = frame->link;
				else lsp->mem->brooding_frame = frame->link;
				sse_lsp_frame_free (frame);
				return SSE_NULL;
			}

			if (v != lsp->mem->nil) {
				if (SSE_LSP_CDR(v) != lsp->mem->nil) {
					lsp->errnum = SSE_LSP_ERR_TOO_MANY_ARGS; // must be a symbol
					if (sequential) lsp->mem->frame = frame->link;
					else lsp->mem->brooding_frame = frame->link;
					sse_lsp_frame_free (frame);
					return SSE_NULL;
				}
				if ((v = sse_lsp_eval(lsp, SSE_LSP_CAR(v))) == SSE_NULL) {
					if (sequential) lsp->mem->frame = frame->link;
					else lsp->mem->brooding_frame = frame->link;
					sse_lsp_frame_free (frame);
					return SSE_NULL;
				}
			}

			if (sse_lsp_frame_lookup (frame, n) != SSE_NULL) {
				lsp->errnum = SSE_LSP_ERR_DUP_FORMAL;
				if (sequential) lsp->mem->frame = frame->link;
				else lsp->mem->brooding_frame = frame->link;
				sse_lsp_frame_free (frame);
				return SSE_NULL;
			}
			if (sse_lsp_frame_insert_value(frame, n, v) == SSE_NULL) {
				lsp->errnum = SSE_LSP_ERR_MEMORY;
				if (sequential) lsp->mem->frame = frame->link;
				else lsp->mem->brooding_frame = frame->link;
				sse_lsp_frame_free (frame);
				return SSE_NULL;
			}
		}
		else if (SSE_LSP_TYPE(ass) == SSE_LSP_OBJ_SYMBOL) {
			if (sse_lsp_frame_lookup(frame, ass) != SSE_NULL) {
				lsp->errnum = SSE_LSP_ERR_DUP_FORMAL;
				if (sequential) lsp->mem->frame = frame->link;
				else lsp->mem->brooding_frame = frame->link;
				sse_lsp_frame_free (frame);
				return SSE_NULL;
			}
			if (sse_lsp_frame_insert_value(frame, ass, lsp->mem->nil) == SSE_NULL) {
				lsp->errnum = SSE_LSP_ERR_MEMORY;
				if (sequential) lsp->mem->frame = frame->link;
				else lsp->mem->brooding_frame = frame->link;
				sse_lsp_frame_free (frame);
				return SSE_NULL;
			}
		}
		else {
			lsp->errnum = SSE_LSP_ERR_BAD_ARG;		
			if (sequential) lsp->mem->frame = frame->link;
			else lsp->mem->brooding_frame = frame->link;
			sse_lsp_frame_free (frame);
			return SSE_NULL;
		}

		assoc = SSE_LSP_CDR(assoc);
	}

	if (assoc != lsp->mem->nil) {
		lsp->errnum = SSE_LSP_ERR_BAD_ARG;	
		if (sequential) lsp->mem->frame = frame->link;
		else lsp->mem->brooding_frame = frame->link;
		sse_lsp_frame_free (frame);
		return SSE_NULL;
	}

	// push the frame
	if (!sequential) {
		lsp->mem->brooding_frame = frame->link;
		frame->link = lsp->mem->frame;
		lsp->mem->frame = frame;
	}

	// evaluate forms in the body
	value = lsp->mem->nil;
	body = SSE_LSP_CDR(args);
	while (body != lsp->mem->nil) {
		value = sse_lsp_eval (lsp, SSE_LSP_CAR(body));
		if (value == SSE_NULL) {
			lsp->mem->frame = frame->link;
			sse_lsp_frame_free (frame);
			return SSE_NULL;
		}
		body = SSE_LSP_CDR(body);
	}

	// pop the frame
	lsp->mem->frame = frame->link;

	// destroy the frame
	sse_lsp_frame_free (frame);
	return value;
}

sse_lsp_obj_t* sse_lsp_prim_let (sse_lsp_t* lsp, sse_lsp_obj_t* args)
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

sse_lsp_obj_t* sse_lsp_prim_letx (sse_lsp_t* lsp, sse_lsp_obj_t* args)
{
	return __prim_let (lsp, args, 1);
}
