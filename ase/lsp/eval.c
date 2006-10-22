/*
 * $Id: eval.c,v 1.14 2006-10-22 13:10:45 bacon Exp $
 */

#include <sse/lsp/lsp.h>
#include <sse/lsp/env.h>
#include <sse/lsp/prim.h>
#include <sse/bas/assert.h>

static sse_lsp_obj_t* make_func (
	sse_lsp_t* lsp, sse_lsp_obj_t* cdr, int is_macro);
static sse_lsp_obj_t* eval_cons (
	sse_lsp_t* lsp, sse_lsp_obj_t* cons);
static sse_lsp_obj_t* apply (
	sse_lsp_t* lsp, sse_lsp_obj_t* func, sse_lsp_obj_t* actual);

sse_lsp_obj_t* sse_lsp_eval (sse_lsp_t* lsp, sse_lsp_obj_t* obj)
{
	lsp->errnum = SSE_LSP_ERR_NONE;

	if (SSE_LSP_TYPE(obj) == SSE_LSP_OBJ_CONS) 
		return eval_cons (lsp, obj);
	else if (SSE_LSP_TYPE(obj) == SSE_LSP_OBJ_SYMBOL) {
		sse_lsp_assoc_t* assoc; 

		/*
		if (obj == lsp->mem->lambda || obj == lsp->mem->macro) {
			printf ("lambda or macro can't be used as a normal symbol\n");
			lsp->errnum = SSE_LSP_ERR_BAD_SYMBOL;
			return SSE_NULL;
		}
		*/

		assoc = sse_lsp_lookup(lsp->mem, obj);
		if (assoc == SSE_NULL || assoc->value == SSE_NULL) {
			if (lsp->opt_undef_symbol) {
				lsp->errnum = SSE_LSP_ERR_UNDEF_SYMBOL;
				return SSE_NULL;
			}
			return lsp->mem->nil;
		}

		obj = assoc->value;
	}

	return obj;
}

static sse_lsp_obj_t* make_func (sse_lsp_t* lsp, sse_lsp_obj_t* cdr, int is_macro)
{
	sse_lsp_obj_t* func, * formal, * body, * p;

	if (cdr == lsp->mem->nil) {
		lsp->errnum = SSE_LSP_ERR_TOO_FEW_ARGS;
		return SSE_NULL;
	}

	if (SSE_LSP_TYPE(cdr) != SSE_LSP_OBJ_CONS) {
		lsp->errnum = SSE_LSP_ERR_BAD_ARG;
		return SSE_NULL;
	}

	formal = SSE_LSP_CAR(cdr);
	body = SSE_LSP_CDR(cdr);

	if (body == lsp->mem->nil) {
		lsp->errnum = SSE_LSP_ERR_EMPTY_BODY;
		return SSE_NULL;
	}

// TODO: more lambda expression syntax checks required???.

	/* check if the lambda express has non-nil value 
	 * at the terminating cdr */
	for (p = body; SSE_LSP_TYPE(p) == SSE_LSP_OBJ_CONS; p = SSE_LSP_CDR(p));
	if (p != lsp->mem->nil) {
		/* like in (lambda (x) (+ x 10) . 4) */
		lsp->errnum = SSE_LSP_ERR_BAD_ARG;
		return SSE_NULL;
	}

	func = (is_macro)?
		sse_lsp_make_macro (lsp->mem, formal, body):
		sse_lsp_make_func (lsp->mem, formal, body);
	if (func == SSE_NULL) {
		lsp->errnum = SSE_LSP_ERR_MEMORY;
		return SSE_NULL;
	}

	return func;
}

static sse_lsp_obj_t* eval_cons (sse_lsp_t* lsp, sse_lsp_obj_t* cons)
{
	sse_lsp_obj_t* car, * cdr;
   
	sse_assert (SSE_LSP_TYPE(cons) == SSE_LSP_OBJ_CONS);

	car = SSE_LSP_CAR(cons);
	cdr = SSE_LSP_CDR(cons);

	if (car == lsp->mem->lambda) {
		return make_func (lsp, cdr, 0);
	}
	else if (car == lsp->mem->macro) {
		return make_func (lsp, cdr, 1);
	}
	else if (SSE_LSP_TYPE(car) == SSE_LSP_OBJ_SYMBOL) {
		sse_lsp_assoc_t* assoc;

		if ((assoc = sse_lsp_lookup(lsp->mem, car)) != SSE_NULL) {
			//sse_lsp_obj_t* func = assoc->value;
			sse_lsp_obj_t* func = assoc->func;
			if (func == SSE_NULL) {
				/* the symbol's function definition is void */
				lsp->errnum = SSE_LSP_ERR_UNDEF_FUNC;
				return SSE_NULL;
			}

			if (SSE_LSP_TYPE(func) == SSE_LSP_OBJ_FUNC ||
			    SSE_LSP_TYPE(func) == SSE_LSP_OBJ_MACRO) {
				return apply (lsp, func, cdr);
			}
			else if (SSE_LSP_TYPE(func) == SSE_LSP_OBJ_PRIM) {
				/* primitive function */
				return SSE_LSP_PRIM(func) (lsp, cdr);
			}
			else {
//TODO: emit the name for debugging
				lsp->errnum = SSE_LSP_ERR_UNDEF_FUNC;
				return SSE_NULL;
			}
		}
		else {
			//TODO: better error handling.
//TODO: emit the name for debugging
			lsp->errnum = SSE_LSP_ERR_UNDEF_FUNC;
			return SSE_NULL;
		}
	}
	else if (SSE_LSP_TYPE(car) == SSE_LSP_OBJ_FUNC || 
	         SSE_LSP_TYPE(car) == SSE_LSP_OBJ_MACRO) {
		return apply (lsp, car, cdr);
	}
	else if (SSE_LSP_TYPE(car) == SSE_LSP_OBJ_CONS) {
		if (SSE_LSP_CAR(car) == lsp->mem->lambda) {
			sse_lsp_obj_t* func = make_func (lsp, SSE_LSP_CDR(car), 0);
			if (func == SSE_NULL) return SSE_NULL;
			return apply (lsp, func, cdr);
		}
		else if (SSE_LSP_CAR(car) == lsp->mem->macro) {
			sse_lsp_obj_t* func = make_func (lsp, SSE_LSP_CDR(car), 1);
			if (func == SSE_NULL) return SSE_NULL;
			return apply (lsp, func, cdr);
		}
	}

//TODO: emit the name for debugging
	lsp->errnum = SSE_LSP_ERR_BAD_FUNC;
	return SSE_NULL;
}

static sse_lsp_obj_t* apply (
	sse_lsp_t* lsp, sse_lsp_obj_t* func, sse_lsp_obj_t* actual)
{
	sse_lsp_frame_t* frame;
	sse_lsp_obj_t* formal;
	sse_lsp_obj_t* body;
	sse_lsp_obj_t* value;
	sse_lsp_mem_t* mem;

	sse_assert (
		SSE_LSP_TYPE(func) == SSE_LSP_OBJ_FUNC ||
		SSE_LSP_TYPE(func) == SSE_LSP_OBJ_MACRO);

	sse_assert (SSE_LSP_TYPE(SSE_LSP_CDR(func)) == SSE_LSP_OBJ_CONS);

	mem = lsp->mem;

	if (SSE_LSP_TYPE(func) == SSE_LSP_OBJ_MACRO) {
		formal = SSE_LSP_MFORMAL (func);
		body   = SSE_LSP_MBODY   (func);
	}
	else {
		formal = SSE_LSP_FFORMAL (func);
		body   = SSE_LSP_FBODY   (func);
	}

	// make a new frame.
	frame = sse_lsp_frame_new ();
	if (frame == SSE_NULL) {
		lsp->errnum = SSE_LSP_ERR_MEMORY;
		return SSE_NULL;
	}

	// attach it to the brooding frame list to 
	// make them not to be garbage collected.
	frame->link = mem->brooding_frame;
	mem->brooding_frame = frame;

	// evaluate arguments and push them into the frame.
	while (formal != mem->nil) {
		if (actual == mem->nil) {
			lsp->errnum = SSE_LSP_ERR_TOO_FEW_ARGS;
			mem->brooding_frame = frame->link;
			sse_lsp_frame_free (frame);
			return SSE_NULL;
		}

		value = SSE_LSP_CAR(actual);
		if (SSE_LSP_TYPE(func) != SSE_LSP_OBJ_MACRO) {
			// macro doesn't evaluate actual arguments.
			value = sse_lsp_eval (lsp, value);
			if (value == SSE_NULL) {
				mem->brooding_frame = frame->link;
				sse_lsp_frame_free (frame);
				return SSE_NULL;
			}
		}

		if (sse_lsp_frame_lookup (
			frame, SSE_LSP_CAR(formal)) != SSE_NULL) {

			lsp->errnum = SSE_LSP_ERR_DUP_FORMAL;
			mem->brooding_frame = frame->link;
			sse_lsp_frame_free (frame);
			return SSE_NULL;
		}

		if (sse_lsp_frame_insert_value (
			frame, SSE_LSP_CAR(formal), value) == SSE_NULL) {

			lsp->errnum = SSE_LSP_ERR_MEMORY;
			mem->brooding_frame = frame->link;
			sse_lsp_frame_free (frame);
			return SSE_NULL;
		}

		actual = SSE_LSP_CDR(actual);
		formal = SSE_LSP_CDR(formal);
	}

	if (SSE_LSP_TYPE(actual) == SSE_LSP_OBJ_CONS) {
		lsp->errnum = SSE_LSP_ERR_TOO_MANY_ARGS;
		mem->brooding_frame = frame->link;
		sse_lsp_frame_free (frame);
		return SSE_NULL;
	}
	else if (actual != mem->nil) {
		lsp->errnum = SSE_LSP_ERR_BAD_ARG;
		mem->brooding_frame = frame->link;
		sse_lsp_frame_free (frame);
		return SSE_NULL;
	}

	// push the frame
	mem->brooding_frame = frame->link;
	frame->link = mem->frame;
	mem->frame = frame;

	// do the evaluation of the body
	value = mem->nil;
	while (body != mem->nil) {
		value = sse_lsp_eval(lsp, SSE_LSP_CAR(body));
		if (value == SSE_NULL) {
			mem->frame = frame->link;
			sse_lsp_frame_free (frame);
			return SSE_NULL;
		}
		body = SSE_LSP_CDR(body);
	}

	// pop the frame.
	mem->frame = frame->link;

	// destroy the frame.
	sse_lsp_frame_free (frame);

	//if (SSE_LSP_CAR(func) == mem->macro) {
	if (SSE_LSP_TYPE(func) == SSE_LSP_OBJ_MACRO) {
		value = sse_lsp_eval(lsp, value);
		if (value == SSE_NULL) return SSE_NULL;
	}

	return value;
}

