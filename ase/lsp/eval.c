/*
 * $Id: eval.c,v 1.16 2006-10-25 13:42:31 bacon Exp $
 */

#include <ase/lsp/lsp.h>
#include <ase/lsp/env.h>
#include <ase/lsp/prim.h>
#include <ase/bas/assert.h>

static ase_lsp_obj_t* make_func (
	ase_lsp_t* lsp, ase_lsp_obj_t* cdr, int is_macro);
static ase_lsp_obj_t* eval_cons (
	ase_lsp_t* lsp, ase_lsp_obj_t* cons);
static ase_lsp_obj_t* apply (
	ase_lsp_t* lsp, ase_lsp_obj_t* func, ase_lsp_obj_t* actual);

ase_lsp_obj_t* ase_lsp_eval (ase_lsp_t* lsp, ase_lsp_obj_t* obj)
{
	lsp->errnum = ASE_LSP_ERR_NONE;

	if (ASE_LSP_TYPE(obj) == ASE_LSP_OBJ_CONS) 
		return eval_cons (lsp, obj);
	else if (ASE_LSP_TYPE(obj) == ASE_LSP_OBJ_SYM) {
		ase_lsp_assoc_t* assoc; 

		/*
		if (obj == lsp->mem->lambda || obj == lsp->mem->macro) {
			printf ("lambda or macro can't be used as a normal symbol\n");
			lsp->errnum = ASE_LSP_ERR_BAD_SYMBOL;
			return ASE_NULL;
		}
		*/

		assoc = ase_lsp_lookup(lsp->mem, obj);
		if (assoc == ASE_NULL || assoc->value == ASE_NULL) {
			if (lsp->opt_undef_symbol) {
				lsp->errnum = ASE_LSP_ERR_UNDEF_SYMBOL;
				return ASE_NULL;
			}
			return lsp->mem->nil;
		}

		obj = assoc->value;
	}

	return obj;
}

static ase_lsp_obj_t* make_func (ase_lsp_t* lsp, ase_lsp_obj_t* cdr, int is_macro)
{
	ase_lsp_obj_t* func, * formal, * body, * p;

	if (cdr == lsp->mem->nil) {
		lsp->errnum = ASE_LSP_ERR_TOO_FEW_ARGS;
		return ASE_NULL;
	}

	if (ASE_LSP_TYPE(cdr) != ASE_LSP_OBJ_CONS) {
		lsp->errnum = ASE_LSP_ERR_BAD_ARG;
		return ASE_NULL;
	}

	formal = ASE_LSP_CAR(cdr);
	body = ASE_LSP_CDR(cdr);

	if (body == lsp->mem->nil) {
		lsp->errnum = ASE_LSP_ERR_EMPTY_BODY;
		return ASE_NULL;
	}

// TODO: more lambda expression syntax checks required???.

	/* check if the lambda express has non-nil value 
	 * at the terminating cdr */
	for (p = body; ASE_LSP_TYPE(p) == ASE_LSP_OBJ_CONS; p = ASE_LSP_CDR(p));
	if (p != lsp->mem->nil) {
		/* like in (lambda (x) (+ x 10) . 4) */
		lsp->errnum = ASE_LSP_ERR_BAD_ARG;
		return ASE_NULL;
	}

	func = (is_macro)?
		ase_lsp_makemacro (lsp->mem, formal, body):
		ase_lsp_makefunc (lsp->mem, formal, body);
	if (func == ASE_NULL) {
		lsp->errnum = ASE_LSP_ERR_MEMORY;
		return ASE_NULL;
	}

	return func;
}

static ase_lsp_obj_t* eval_cons (ase_lsp_t* lsp, ase_lsp_obj_t* cons)
{
	ase_lsp_obj_t* car, * cdr;
   
	ase_assert (ASE_LSP_TYPE(cons) == ASE_LSP_OBJ_CONS);

	car = ASE_LSP_CAR(cons);
	cdr = ASE_LSP_CDR(cons);

	if (car == lsp->mem->lambda) 
	{
		return make_func (lsp, cdr, 0);
	}
	else if (car == lsp->mem->macro) 
	{
		return make_func (lsp, cdr, 1);
	}
	else if (ASE_LSP_TYPE(car) == ASE_LSP_OBJ_SYM) 
	{
		ase_lsp_assoc_t* assoc;

		if ((assoc = ase_lsp_lookup(lsp->mem, car)) != ASE_NULL) 
		{
			//ase_lsp_obj_t* func = assoc->value;
			ase_lsp_obj_t* func = assoc->func;
			if (func == ASE_NULL) {
				/* the symbol's function definition is void */
				lsp->errnum = ASE_LSP_ERR_UNDEF_FUNC;
				return ASE_NULL;
			}

			if (ASE_LSP_TYPE(func) == ASE_LSP_OBJ_FUNC ||
			    ASE_LSP_TYPE(func) == ASE_LSP_OBJ_MACRO) 
			{
				return apply (lsp, func, cdr);
			}
			else if (ASE_LSP_TYPE(func) == ASE_LSP_OBJ_PRIM) 
			{
				/* primitive function */
				return ASE_LSP_PRIM(func) (lsp, cdr);
			}
			else 
			{
//TODO: emit the name for debugging
				lsp->errnum = ASE_LSP_ERR_UNDEF_FUNC;
				return ASE_NULL;
			}
		}
		else {
			//TODO: better error handling.
//TODO: emit the name for debugging
			lsp->errnum = ASE_LSP_ERR_UNDEF_FUNC;
			return ASE_NULL;
		}
	}
	else if (ASE_LSP_TYPE(car) == ASE_LSP_OBJ_FUNC || 
	         ASE_LSP_TYPE(car) == ASE_LSP_OBJ_MACRO) {
		return apply (lsp, car, cdr);
	}
	else if (ASE_LSP_TYPE(car) == ASE_LSP_OBJ_CONS) {
		if (ASE_LSP_CAR(car) == lsp->mem->lambda) {
			ase_lsp_obj_t* func = make_func (lsp, ASE_LSP_CDR(car), 0);
			if (func == ASE_NULL) return ASE_NULL;
			return apply (lsp, func, cdr);
		}
		else if (ASE_LSP_CAR(car) == lsp->mem->macro) {
			ase_lsp_obj_t* func = make_func (lsp, ASE_LSP_CDR(car), 1);
			if (func == ASE_NULL) return ASE_NULL;
			return apply (lsp, func, cdr);
		}
	}

//TODO: emit the name for debugging
	lsp->errnum = ASE_LSP_ERR_BAD_FUNC;
	return ASE_NULL;
}

static ase_lsp_obj_t* apply (
	ase_lsp_t* lsp, ase_lsp_obj_t* func, ase_lsp_obj_t* actual)
{
	ase_lsp_frame_t* frame;
	ase_lsp_obj_t* formal;
	ase_lsp_obj_t* body;
	ase_lsp_obj_t* value;
	ase_lsp_mem_t* mem;

	ase_assert (
		ASE_LSP_TYPE(func) == ASE_LSP_OBJ_FUNC ||
		ASE_LSP_TYPE(func) == ASE_LSP_OBJ_MACRO);

	ase_assert (ASE_LSP_TYPE(ASE_LSP_CDR(func)) == ASE_LSP_OBJ_CONS);

	mem = lsp->mem;

	if (ASE_LSP_TYPE(func) == ASE_LSP_OBJ_MACRO) {
		formal = ASE_LSP_MFORMAL (func);
		body   = ASE_LSP_MBODY   (func);
	}
	else {
		formal = ASE_LSP_FFORMAL (func);
		body   = ASE_LSP_FBODY   (func);
	}

	// make a new frame.
	frame = ase_lsp_frame_new ();
	if (frame == ASE_NULL) {
		lsp->errnum = ASE_LSP_ERR_MEMORY;
		return ASE_NULL;
	}

	// attach it to the brooding frame list to 
	// make them not to be garbage collected.
	frame->link = mem->brooding_frame;
	mem->brooding_frame = frame;

	// evaluate arguments and push them into the frame.
	while (formal != mem->nil) {
		if (actual == mem->nil) {
			lsp->errnum = ASE_LSP_ERR_TOO_FEW_ARGS;
			mem->brooding_frame = frame->link;
			ase_lsp_frame_free (frame);
			return ASE_NULL;
		}

		value = ASE_LSP_CAR(actual);
		if (ASE_LSP_TYPE(func) != ASE_LSP_OBJ_MACRO) {
			// macro doesn't evaluate actual arguments.
			value = ase_lsp_eval (lsp, value);
			if (value == ASE_NULL) {
				mem->brooding_frame = frame->link;
				ase_lsp_frame_free (frame);
				return ASE_NULL;
			}
		}

		if (ase_lsp_frame_lookup (
			frame, ASE_LSP_CAR(formal)) != ASE_NULL) {

			lsp->errnum = ASE_LSP_ERR_DUP_FORMAL;
			mem->brooding_frame = frame->link;
			ase_lsp_frame_free (frame);
			return ASE_NULL;
		}

		if (ase_lsp_frame_insert_value (
			frame, ASE_LSP_CAR(formal), value) == ASE_NULL) {

			lsp->errnum = ASE_LSP_ERR_MEMORY;
			mem->brooding_frame = frame->link;
			ase_lsp_frame_free (frame);
			return ASE_NULL;
		}

		actual = ASE_LSP_CDR(actual);
		formal = ASE_LSP_CDR(formal);
	}

	if (ASE_LSP_TYPE(actual) == ASE_LSP_OBJ_CONS) {
		lsp->errnum = ASE_LSP_ERR_TOO_MANY_ARGS;
		mem->brooding_frame = frame->link;
		ase_lsp_frame_free (frame);
		return ASE_NULL;
	}
	else if (actual != mem->nil) {
		lsp->errnum = ASE_LSP_ERR_BAD_ARG;
		mem->brooding_frame = frame->link;
		ase_lsp_frame_free (frame);
		return ASE_NULL;
	}

	// push the frame
	mem->brooding_frame = frame->link;
	frame->link = mem->frame;
	mem->frame = frame;

	// do the evaluation of the body
	value = mem->nil;
	while (body != mem->nil) {
		value = ase_lsp_eval(lsp, ASE_LSP_CAR(body));
		if (value == ASE_NULL) {
			mem->frame = frame->link;
			ase_lsp_frame_free (frame);
			return ASE_NULL;
		}
		body = ASE_LSP_CDR(body);
	}

	// pop the frame.
	mem->frame = frame->link;

	// destroy the frame.
	ase_lsp_frame_free (frame);

	//if (ASE_LSP_CAR(func) == mem->macro) {
	if (ASE_LSP_TYPE(func) == ASE_LSP_OBJ_MACRO) {
		value = ase_lsp_eval(lsp, value);
		if (value == ASE_NULL) return ASE_NULL;
	}

	return value;
}

