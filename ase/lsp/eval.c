/*
 * $Id: eval.c,v 1.3 2005-02-04 16:23:34 bacon Exp $
 */

#include <xp/lisp/lisp.h>
#include <xp/lisp/env.h>
#include <xp/lisp/primitive.h>

#ifdef __cplusplus
extern "C" {
#endif

static xp_lisp_obj_t* make_func (xp_lisp_t* lsp, xp_lisp_obj_t* cdr, int is_macro);
static xp_lisp_obj_t* eval_cons (xp_lisp_t* lsp, xp_lisp_obj_t* cons);
static xp_lisp_obj_t* apply     (xp_lisp_t* lsp, xp_lisp_obj_t* func, xp_lisp_obj_t* actual);

#ifdef __cplusplus
}
#endif

xp_lisp_obj_t* xp_lisp_eval (xp_lisp_t* lsp, xp_lisp_obj_t* obj)
{
	lsp->error = XP_LISP_ERR_NONE;

	if (XP_LISP_TYPE(obj) == XP_LISP_OBJ_CONS) 
		return eval_cons (lsp, obj);
	else if (XP_LISP_TYPE(obj) == XP_LISP_OBJ_SYMBOL) {
		xp_lisp_assoc_t* assoc; 

		/*
		if (obj == lsp->mem->lambda || obj == lsp->mem->macro) {
			printf ("lambda or macro can't be used as a normal symbol\n");
			lsp->error = XP_LISP_ERR_BAD_SYMBOL;
			return XP_NULL;
		}
		*/

		if ((assoc = xp_lisp_lookup (lsp->mem, obj)) == XP_NULL) {
			if (lsp->opt_undef_symbol) {
				lsp->error = XP_LISP_ERR_UNDEF_SYMBOL;
				return XP_NULL;
			}
			return lsp->mem->nil;
		}

		obj = assoc->value;
	}

	return obj;
}

static xp_lisp_obj_t* make_func (xp_lisp_t* lsp, xp_lisp_obj_t* cdr, int is_macro)
{
	// TODO: lambda expression syntax check.
	xp_lisp_obj_t* func, * formal, * body;

	printf ("about to create a function or a macro ....\n");

	if (cdr == lsp->mem->nil) {
		lsp->error = XP_LISP_ERR_TOO_FEW_ARGS;
		return XP_NULL;
	}

	if (XP_LISP_TYPE(cdr) != XP_LISP_OBJ_CONS) {
		lsp->error = XP_LISP_ERR_BAD_ARG;
		return XP_NULL;
	}

	formal = XP_LISP_CAR(cdr);
	body = XP_LISP_CDR(cdr);

	if (body == lsp->mem->nil) {
		lsp->error = XP_LISP_ERR_EMPTY_BODY;
		return XP_NULL;
	}

	func = (is_macro)?
		xp_lisp_make_macro (lsp->mem, formal, body):
		xp_lisp_make_func (lsp->mem, formal, body);
	if (func == XP_NULL) {
		lsp->error = XP_LISP_ERR_MEM;
		return XP_NULL;
	}

	return func;
}

static xp_lisp_obj_t* eval_cons (xp_lisp_t* lsp, xp_lisp_obj_t* cons)
{
	xp_lisp_obj_t* car, * cdr;
   
	xp_lisp_assert (XP_LISP_TYPE(cons) == XP_LISP_OBJ_CONS);

	car = XP_LISP_CAR(cons);
	cdr = XP_LISP_CDR(cons);

	if (car == lsp->mem->lambda) {
		return make_func (lsp, cdr, 0);
	}
	else if (car == lsp->mem->macro) {
		return make_func (lsp, cdr, 1);
	}
	else if (XP_LISP_TYPE(car) == XP_LISP_OBJ_SYMBOL) {
		xp_lisp_assoc_t* assoc;

		if ((assoc = xp_lisp_lookup (lsp->mem, car)) != XP_NULL) {
			xp_lisp_obj_t* func = assoc->value;
			if (XP_LISP_TYPE(func) == XP_LISP_OBJ_FUNC ||
			    XP_LISP_TYPE(func) == XP_LISP_OBJ_MACRO) {
				return apply (lsp, func, cdr);
			}
			else if (XP_LISP_TYPE(func) == XP_LISP_OBJ_PRIM) {
				// primitive function
				return XP_LISP_PIMPL(func) (lsp, cdr);
			}
			else {
				printf ("undefined function: ");
				xp_lisp_print (lsp, car);
				printf ("\n");
				lsp->error = XP_LISP_ERR_UNDEF_FUNC;
				return XP_NULL;
			}
		}
		else {
			//TODO: better error handling.
			printf ("undefined function: ");
			xp_lisp_print (lsp, car);
			printf ("\n");
			lsp->error = XP_LISP_ERR_UNDEF_FUNC;
			return XP_NULL;
		}
	}
	else if (XP_LISP_TYPE(car) == XP_LISP_OBJ_FUNC || 
	         XP_LISP_TYPE(car) == XP_LISP_OBJ_MACRO) {
		return apply (lsp, car, cdr);
	}
	else if (XP_LISP_TYPE(car) == XP_LISP_OBJ_CONS) {
		if (XP_LISP_CAR(car) == lsp->mem->lambda) {
			xp_lisp_obj_t* func = make_func (lsp, XP_LISP_CDR(car), 0);
			if (func == XP_NULL) return XP_NULL;
			return apply (lsp, func, cdr);
		}
		else if (XP_LISP_CAR(car) == lsp->mem->macro) {
			xp_lisp_obj_t* func = make_func (lsp, XP_LISP_CDR(car), 1);
			if (func == XP_NULL) return XP_NULL;
			return apply (lsp, func, cdr);
		}
	}

	xp_printf (XP_TEXT("bad function: "));
	xp_lisp_print (lsp, car);
	xp_printf (XP_TEXT("\n"));
	lsp->error = XP_LISP_ERR_BAD_FUNC;
	return XP_NULL;
}

static xp_lisp_obj_t* apply (xp_lisp_t* lsp, xp_lisp_obj_t* func, xp_lisp_obj_t* actual)
{
	xp_lisp_frame_t* frame;
	xp_lisp_obj_t* formal;
	xp_lisp_obj_t* body;
	xp_lisp_obj_t* value;
	xp_lisp_mem_t* mem;

	xp_lisp_assert (
		XP_LISP_TYPE(func) == XP_LISP_OBJ_FUNC ||
		XP_LISP_TYPE(func) == XP_LISP_OBJ_MACRO);

	xp_lisp_assert (XP_LISP_TYPE(XP_LISP_CDR(func)) == XP_LISP_OBJ_CONS);

	mem = lsp->mem;

	if (XP_LISP_TYPE(func) == XP_LISP_OBJ_MACRO) {
		formal = XP_LISP_MFORMAL (func);
		body   = XP_LISP_MBODY   (func);
	}
	else {
		formal = XP_LISP_FFORMAL (func);
		body   = XP_LISP_FBODY   (func);
	}

	// make a new frame.
	frame = xp_lisp_frame_new ();
	if (frame == XP_NULL) {
		lsp->error = XP_LISP_ERR_MEM;
		return XP_NULL;
	}

	// attach it to the brooding frame list to 
	// make them not to be garbage collected.
	frame->link = mem->brooding_frame;
	mem->brooding_frame = frame;

	// evaluate arguments and push them into the frame.
	while (formal != mem->nil) {
		if (actual == mem->nil) {
			lsp->error = XP_LISP_ERR_TOO_FEW_ARGS;
			mem->brooding_frame = frame->link;
			xp_lisp_frame_free (frame);
			return XP_NULL;
		}

		value = XP_LISP_CAR(actual);
		if (XP_LISP_TYPE(func) != XP_LISP_OBJ_MACRO) {
			// macro doesn't evaluate actual arguments.
			value = xp_lisp_eval (lsp, value);
			if (value == XP_NULL) {
				mem->brooding_frame = frame->link;
				xp_lisp_frame_free (frame);
				return XP_NULL;
			}
		}

		if (xp_lisp_frame_lookup (frame, XP_LISP_CAR(formal)) != XP_NULL) {
			lsp->error = XP_LISP_ERR_DUP_FORMAL;
			mem->brooding_frame = frame->link;
			xp_lisp_frame_free (frame);
			return XP_NULL;
		}
		if (xp_lisp_frame_insert (frame, XP_LISP_CAR(formal), value) == XP_NULL) {
			lsp->error = XP_LISP_ERR_MEM;
			mem->brooding_frame = frame->link;
			xp_lisp_frame_free (frame);
			return XP_NULL;
		}

		actual = XP_LISP_CDR(actual);
		formal = XP_LISP_CDR(formal);
	}

	if (XP_LISP_TYPE(actual) == XP_LISP_OBJ_CONS) {
		lsp->error = XP_LISP_ERR_TOO_MANY_ARGS;
		mem->brooding_frame = frame->link;
		xp_lisp_frame_free (frame);
		return XP_NULL;
	}
	else if (actual != mem->nil) {
		lsp->error = XP_LISP_ERR_BAD_ARG;
		mem->brooding_frame = frame->link;
		xp_lisp_frame_free (frame);
		return XP_NULL;
	}

	// push the frame
	mem->brooding_frame = frame->link;
	frame->link = mem->frame;
	mem->frame = frame;

	// do the evaluation of the body
	value = mem->nil;
	while (body != mem->nil) {
		value = xp_lisp_eval(lsp, XP_LISP_CAR(body));
		if (value == XP_NULL) {
			mem->frame = frame->link;
			xp_lisp_frame_free (frame);
			return XP_NULL;
		}
		body = XP_LISP_CDR(body);
	}

	// pop the frame.
	mem->frame = frame->link;

	// destroy the frame.
	xp_lisp_frame_free (frame);

	//if (XP_LISP_CAR(func) == mem->macro) {
	if (XP_LISP_TYPE(func) == XP_LISP_OBJ_MACRO) {
		value = xp_lisp_eval(lsp, value);
		if (value == XP_NULL) return XP_NULL;
	}

	return value;
}

