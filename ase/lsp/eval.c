/*
 * $Id: eval.c,v 1.11 2005-09-20 09:17:06 bacon Exp $
 */

#include <xp/lsp/lsp.h>
#include <xp/lsp/env.h>
#include <xp/lsp/prim.h>
#include <xp/bas/assert.h>

static xp_lsp_obj_t* make_func (
	xp_lsp_t* lsp, xp_lsp_obj_t* cdr, int is_macro);
static xp_lsp_obj_t* eval_cons (
	xp_lsp_t* lsp, xp_lsp_obj_t* cons);
static xp_lsp_obj_t* apply (
	xp_lsp_t* lsp, xp_lsp_obj_t* func, xp_lsp_obj_t* actual);

xp_lsp_obj_t* xp_lsp_eval (xp_lsp_t* lsp, xp_lsp_obj_t* obj)
{
	lsp->errnum = XP_LSP_ERR_NONE;

	if (XP_LSP_TYPE(obj) == XP_LSP_OBJ_CONS) 
		return eval_cons (lsp, obj);
	else if (XP_LSP_TYPE(obj) == XP_LSP_OBJ_SYMBOL) {
		xp_lsp_assoc_t* assoc; 

		/*
		if (obj == lsp->mem->lambda || obj == lsp->mem->macro) {
			printf ("lambda or macro can't be used as a normal symbol\n");
			lsp->errnum = XP_LSP_ERR_BAD_SYMBOL;
			return XP_NULL;
		}
		*/

		if ((assoc = xp_lsp_lookup(lsp->mem, obj)) == XP_NULL) {
			if (lsp->opt_undef_symbol) {
				lsp->errnum = XP_LSP_ERR_UNDEF_SYMBOL;
				return XP_NULL;
			}
			return lsp->mem->nil;
		}

		obj = assoc->value;
	}

	return obj;
}

static xp_lsp_obj_t* make_func (xp_lsp_t* lsp, xp_lsp_obj_t* cdr, int is_macro)
{
	xp_lsp_obj_t* func, * formal, * body, * p;

	if (cdr == lsp->mem->nil) {
		lsp->errnum = XP_LSP_ERR_TOO_FEW_ARGS;
		return XP_NULL;
	}

	if (XP_LSP_TYPE(cdr) != XP_LSP_OBJ_CONS) {
		lsp->errnum = XP_LSP_ERR_BAD_ARG;
		return XP_NULL;
	}

	formal = XP_LSP_CAR(cdr);
	body = XP_LSP_CDR(cdr);

	if (body == lsp->mem->nil) {
		lsp->errnum = XP_LSP_ERR_EMPTY_BODY;
		return XP_NULL;
	}

// TODO: more lambda expression syntax checks required???.

	/* check if the lambda express has non-nil value 
	 * at the terminating cdr */
	for (p = body; XP_LSP_TYPE(p) == XP_LSP_OBJ_CONS; p = XP_LSP_CDR(p));
	if (p != lsp->mem->nil) {
		/* like in (lambda (x) (+ x 10) . 4) */
		lsp->errnum = XP_LSP_ERR_BAD_ARG;
		return XP_NULL;
	}

	func = (is_macro)?
		xp_lsp_make_macro (lsp->mem, formal, body):
		xp_lsp_make_func (lsp->mem, formal, body);
	if (func == XP_NULL) {
		lsp->errnum = XP_LSP_ERR_MEM;
		return XP_NULL;
	}

	return func;
}

static xp_lsp_obj_t* eval_cons (xp_lsp_t* lsp, xp_lsp_obj_t* cons)
{
	xp_lsp_obj_t* car, * cdr;
   
	xp_assert (XP_LSP_TYPE(cons) == XP_LSP_OBJ_CONS);

	car = XP_LSP_CAR(cons);
	cdr = XP_LSP_CDR(cons);

	if (car == lsp->mem->lambda) {
		return make_func (lsp, cdr, 0);
	}
	else if (car == lsp->mem->macro) {
		return make_func (lsp, cdr, 1);
	}
	else if (XP_LSP_TYPE(car) == XP_LSP_OBJ_SYMBOL) {
		xp_lsp_assoc_t* assoc;

		if ((assoc = xp_lsp_lookup(lsp->mem, car)) != XP_NULL) {
			//xp_lsp_obj_t* func = assoc->value;
			xp_lsp_obj_t* func = assoc->func;
			if (func == XP_NULL) {
				/* the symbol's function definition is void */
				lsp->errnum = XP_LSP_ERR_UNDEF_FUNC;
				return XP_NULL;
			}

			if (XP_LSP_TYPE(func) == XP_LSP_OBJ_FUNC ||
			    XP_LSP_TYPE(func) == XP_LSP_OBJ_MACRO) {
				return apply (lsp, func, cdr);
			}
			else if (XP_LSP_TYPE(func) == XP_LSP_OBJ_PRIM) {
				// primitive function
				return XP_LSP_PIMPL(func) (lsp, cdr);
			}
			else {
//TODO: emit the name for debugging
				lsp->errnum = XP_LSP_ERR_UNDEF_FUNC;
				return XP_NULL;
			}
		}
		else {
			//TODO: better error handling.
//TODO: emit the name for debugging
			lsp->errnum = XP_LSP_ERR_UNDEF_FUNC;
			return XP_NULL;
		}
	}
	else if (XP_LSP_TYPE(car) == XP_LSP_OBJ_FUNC || 
	         XP_LSP_TYPE(car) == XP_LSP_OBJ_MACRO) {
		return apply (lsp, car, cdr);
	}
	else if (XP_LSP_TYPE(car) == XP_LSP_OBJ_CONS) {
		if (XP_LSP_CAR(car) == lsp->mem->lambda) {
			xp_lsp_obj_t* func = make_func (lsp, XP_LSP_CDR(car), 0);
			if (func == XP_NULL) return XP_NULL;
			return apply (lsp, func, cdr);
		}
		else if (XP_LSP_CAR(car) == lsp->mem->macro) {
			xp_lsp_obj_t* func = make_func (lsp, XP_LSP_CDR(car), 1);
			if (func == XP_NULL) return XP_NULL;
			return apply (lsp, func, cdr);
		}
	}

//TODO: emit the name for debugging
	lsp->errnum = XP_LSP_ERR_BAD_FUNC;
	return XP_NULL;
}

static xp_lsp_obj_t* apply (
	xp_lsp_t* lsp, xp_lsp_obj_t* func, xp_lsp_obj_t* actual)
{
	xp_lsp_frame_t* frame;
	xp_lsp_obj_t* formal;
	xp_lsp_obj_t* body;
	xp_lsp_obj_t* value;
	xp_lsp_mem_t* mem;

	xp_assert (
		XP_LSP_TYPE(func) == XP_LSP_OBJ_FUNC ||
		XP_LSP_TYPE(func) == XP_LSP_OBJ_MACRO);

	xp_assert (XP_LSP_TYPE(XP_LSP_CDR(func)) == XP_LSP_OBJ_CONS);

	mem = lsp->mem;

	if (XP_LSP_TYPE(func) == XP_LSP_OBJ_MACRO) {
		formal = XP_LSP_MFORMAL (func);
		body   = XP_LSP_MBODY   (func);
	}
	else {
		formal = XP_LSP_FFORMAL (func);
		body   = XP_LSP_FBODY   (func);
	}

	// make a new frame.
	frame = xp_lsp_frame_new ();
	if (frame == XP_NULL) {
		lsp->errnum = XP_LSP_ERR_MEM;
		return XP_NULL;
	}

	// attach it to the brooding frame list to 
	// make them not to be garbage collected.
	frame->link = mem->brooding_frame;
	mem->brooding_frame = frame;

	// evaluate arguments and push them into the frame.
	while (formal != mem->nil) {
		if (actual == mem->nil) {
			lsp->errnum = XP_LSP_ERR_TOO_FEW_ARGS;
			mem->brooding_frame = frame->link;
			xp_lsp_frame_free (frame);
			return XP_NULL;
		}

		value = XP_LSP_CAR(actual);
		if (XP_LSP_TYPE(func) != XP_LSP_OBJ_MACRO) {
			// macro doesn't evaluate actual arguments.
			value = xp_lsp_eval (lsp, value);
			if (value == XP_NULL) {
				mem->brooding_frame = frame->link;
				xp_lsp_frame_free (frame);
				return XP_NULL;
			}
		}

		if (xp_lsp_frame_lookup (
			frame, XP_LSP_CAR(formal)) != XP_NULL) {

			lsp->errnum = XP_LSP_ERR_DUP_FORMAL;
			mem->brooding_frame = frame->link;
			xp_lsp_frame_free (frame);
			return XP_NULL;
		}

		if (xp_lsp_frame_insert_value (
			frame, XP_LSP_CAR(formal), value) == XP_NULL) {

			lsp->errnum = XP_LSP_ERR_MEM;
			mem->brooding_frame = frame->link;
			xp_lsp_frame_free (frame);
			return XP_NULL;
		}

		actual = XP_LSP_CDR(actual);
		formal = XP_LSP_CDR(formal);
	}

	if (XP_LSP_TYPE(actual) == XP_LSP_OBJ_CONS) {
		lsp->errnum = XP_LSP_ERR_TOO_MANY_ARGS;
		mem->brooding_frame = frame->link;
		xp_lsp_frame_free (frame);
		return XP_NULL;
	}
	else if (actual != mem->nil) {
		lsp->errnum = XP_LSP_ERR_BAD_ARG;
		mem->brooding_frame = frame->link;
		xp_lsp_frame_free (frame);
		return XP_NULL;
	}

	// push the frame
	mem->brooding_frame = frame->link;
	frame->link = mem->frame;
	mem->frame = frame;

	// do the evaluation of the body
	value = mem->nil;
	while (body != mem->nil) {
		value = xp_lsp_eval(lsp, XP_LSP_CAR(body));
		if (value == XP_NULL) {
			mem->frame = frame->link;
			xp_lsp_frame_free (frame);
			return XP_NULL;
		}
		body = XP_LSP_CDR(body);
	}

	// pop the frame.
	mem->frame = frame->link;

	// destroy the frame.
	xp_lsp_frame_free (frame);

	//if (XP_LSP_CAR(func) == mem->macro) {
	if (XP_LSP_TYPE(func) == XP_LSP_OBJ_MACRO) {
		value = xp_lsp_eval(lsp, value);
		if (value == XP_NULL) return XP_NULL;
	}

	return value;
}

