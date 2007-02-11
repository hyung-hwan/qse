/*
 * $Id: eval.c,v 1.26 2007-02-11 07:36:54 bacon Exp $
 *
 * {License}
 */

#include <ase/lsp/lsp_i.h>

static ase_lsp_obj_t* __eval (ase_lsp_t* lsp, ase_lsp_obj_t* obj);
static ase_lsp_obj_t* makefn (
	ase_lsp_t* lsp, ase_lsp_obj_t* cdr, int is_macro);
static ase_lsp_obj_t* eval_cons (
	ase_lsp_t* lsp, ase_lsp_obj_t* cons);
static ase_lsp_obj_t* apply (
	ase_lsp_t* lsp, ase_lsp_obj_t* func, ase_lsp_obj_t* actual);
static ase_lsp_obj_t* apply_to_prim (
	ase_lsp_t* lsp, ase_lsp_obj_t* func, ase_lsp_obj_t* actual);

ase_lsp_obj_t* ase_lsp_eval (ase_lsp_t* lsp, ase_lsp_obj_t* obj)
{
	if (ASE_LSP_TYPE(obj) == ASE_LSP_OBJ_CONS) 
	{
		return eval_cons (lsp, obj);
	}
	else if (ASE_LSP_TYPE(obj) == ASE_LSP_OBJ_SYM) 
	{
		ase_lsp_assoc_t* assoc; 

		/*
		if (obj == lsp->mem->lambda || obj == lsp->mem->macro) {
			ase_char_t* arg[1];

			arg[0] = ASE_LSP_SYMPTR(obj);

			printf ("lambda or macro can't be used as a normal symbol\n");
			ase_lsp_seterror (
				lsp, ASE_LSP_EBADSYM, 
				arg, ASE_COUNTOF(arg));
			return ASE_NULL;
		}
		*/

		assoc = ase_lsp_lookup(lsp->mem, obj);
		if (assoc == ASE_NULL || assoc->value == ASE_NULL) 
		{
			if (lsp->opt_undef_symbol) 
			{
				ase_char_t* arg[1];

				arg[0] = ASE_LSP_SYMPTR(obj);

				ase_lsp_seterror (
					lsp, ASE_LSP_EUNDEFSYM, 
					arg, ASE_COUNTOF(arg));
				return ASE_NULL;
			}
			return lsp->mem->nil;
		}

		obj = assoc->value;
	}

	return obj;
}

static ase_lsp_obj_t* makefn (ase_lsp_t* lsp, ase_lsp_obj_t* cdr, int is_macro)
{
	ase_lsp_obj_t* func, * formal, * body, * p;

	if (cdr == lsp->mem->nil) 
	{
		ase_lsp_seterror (lsp, ASE_LSP_EARGFEW, ASE_NULL, 0);
		return ASE_NULL;
	}

	if (ASE_LSP_TYPE(cdr) != ASE_LSP_OBJ_CONS) 
	{
		ase_lsp_seterror (lsp, ASE_LSP_EARGBAD, ASE_NULL, 0);
		return ASE_NULL;
	}

	formal = ASE_LSP_CAR(cdr);
	body = ASE_LSP_CDR(cdr);

	if (body == lsp->mem->nil) 
	{
		ase_lsp_seterror (lsp, ASE_LSP_EEMPBDY, ASE_NULL, 0);
		return ASE_NULL;
	}

// TODO: more lambda expression syntax checks required???.

	/* check if the lambda express has non-nil value 
	 * at the terminating cdr */
	for (p = body; ASE_LSP_TYPE(p) == ASE_LSP_OBJ_CONS; p = ASE_LSP_CDR(p));
	if (p != lsp->mem->nil) 
	{
		/* like in (lambda (x) (+ x 10) . 4) */
		ase_lsp_seterror (lsp, ASE_LSP_EARGBAD, ASE_NULL, 0);
		return ASE_NULL;
	}

	func = (is_macro)?
		ase_lsp_makemacro (lsp->mem, formal, body):
		ase_lsp_makefunc (lsp->mem, formal, body);
	if (func == ASE_NULL) return ASE_NULL;

	return func;
}

static ase_lsp_obj_t* eval_cons (ase_lsp_t* lsp, ase_lsp_obj_t* cons)
{
	ase_lsp_obj_t* car, * cdr;
   
	ASE_LSP_ASSERT (lsp, ASE_LSP_TYPE(cons) == ASE_LSP_OBJ_CONS);

	car = ASE_LSP_CAR(cons);
	cdr = ASE_LSP_CDR(cons);

	if (car == lsp->mem->lambda) 
	{
		/* (lambda (x) (+ x 20)) */
		return makefn (lsp, cdr, 0);
	}
	else if (car == lsp->mem->macro) 
	{
		/* (macro (x) (+ x 20)) */
		return makefn (lsp, cdr, 1);
	}
	else if (ASE_LSP_TYPE(car) == ASE_LSP_OBJ_SYM) 
	{
		ase_lsp_assoc_t* assoc;

		if ((assoc = ase_lsp_lookup(lsp->mem, car)) != ASE_NULL) 
		{
			/*ase_lsp_obj_t* func = assoc->value;*/
			ase_lsp_obj_t* func = assoc->func;
			if (func == ASE_NULL) 
			{
				/* the symbol's function definition is void */
				ase_char_t* arg[1];

				arg[0] = ASE_LSP_SYMPTR(car);
				ase_lsp_seterror (
					lsp, ASE_LSP_EUNDEFFN, 
					arg, ASE_COUNTOF(arg));

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
				return apply_to_prim (lsp, func, cdr);
			}
			else 
			{
				ase_char_t* arg[1];

				arg[0] = ASE_LSP_SYMPTR(car);
				ase_lsp_seterror (
					lsp, ASE_LSP_EUNDEFFN, 
					arg, ASE_COUNTOF(arg));

				return ASE_NULL;
			}
		}
		else 
		{
			ase_char_t* arg[1];

			arg[0] = ASE_LSP_SYMPTR(car);
			ase_lsp_seterror (
				lsp, ASE_LSP_EUNDEFFN, 
				arg, ASE_COUNTOF(arg));

			return ASE_NULL;
		}
	}
	else if (ASE_LSP_TYPE(car) == ASE_LSP_OBJ_FUNC || 
	         ASE_LSP_TYPE(car) == ASE_LSP_OBJ_MACRO) 
	{
		return apply (lsp, car, cdr);
	}
	else if (ASE_LSP_TYPE(car) == ASE_LSP_OBJ_CONS) 
	{
		/* anonymous function or macros 
		 * ((lambda (x) (+ x 10)) 50) */
		if (ASE_LSP_CAR(car) == lsp->mem->lambda) 
		{
			ase_lsp_obj_t* func = makefn (lsp, ASE_LSP_CDR(car), 0);
			if (func == ASE_NULL) return ASE_NULL;
			return apply (lsp, func, cdr);
		}
		else if (ASE_LSP_CAR(car) == lsp->mem->macro) 
		{
			ase_lsp_obj_t* func = makefn (lsp, ASE_LSP_CDR(car), 1);
			if (func == ASE_NULL) return ASE_NULL;
			return apply (lsp, func, cdr);
		}
	}

	ase_lsp_seterror (lsp, ASE_LSP_EBADFN, ASE_NULL, 0);
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

	ASE_LSP_ASSERT (lsp,
		ASE_LSP_TYPE(func) == ASE_LSP_OBJ_FUNC ||
		ASE_LSP_TYPE(func) == ASE_LSP_OBJ_MACRO);

	ASE_LSP_ASSERT (lsp,
		ASE_LSP_TYPE(ASE_LSP_CDR(func)) == ASE_LSP_OBJ_CONS);

	mem = lsp->mem;

	if (ASE_LSP_TYPE(func) == ASE_LSP_OBJ_MACRO) 
	{
		formal = ASE_LSP_MFORMAL (func);
		body   = ASE_LSP_MBODY   (func);
	}
	else 
	{
		formal = ASE_LSP_FFORMAL (func);
		body   = ASE_LSP_FBODY   (func);
	}

	/* make a new frame. */
	frame = ase_lsp_newframe (lsp);
	if (frame == ASE_NULL) return ASE_NULL;

	/* attach it to the brooding frame list to 
	 * prevent them from being garbage-collected. */
	frame->link = mem->brooding_frame;
	mem->brooding_frame = frame;

	/* evaluate arguments and push them into the frame. */
	while (formal != mem->nil) 
	{
		if (actual == mem->nil) 
		{
			mem->brooding_frame = frame->link;
			ase_lsp_freeframe (lsp, frame);

			ase_lsp_seterror (lsp, ASE_LSP_EARGFEW, ASE_NULL, 0);
			return ASE_NULL;
		}

		value = ASE_LSP_CAR(actual);
		if (ASE_LSP_TYPE(func) != ASE_LSP_OBJ_MACRO) 
		{
			/* macro doesn't evaluate actual arguments. */
			value = ase_lsp_eval (lsp, value);
			if (value == ASE_NULL) 
			{
				mem->brooding_frame = frame->link;
				ase_lsp_freeframe (lsp, frame);
				return ASE_NULL;
			}
		}

		if (ase_lsp_lookupinframe (
			lsp, frame, ASE_LSP_CAR(formal)) != ASE_NULL) 
		{
			mem->brooding_frame = frame->link;
			ase_lsp_freeframe (lsp, frame);

			ase_lsp_seterror (lsp, ASE_LSP_EDUPFML, ASE_NULL, 0);
			return ASE_NULL;
		}

		if (ase_lsp_insvalueintoframe (
			lsp, frame, ASE_LSP_CAR(formal), value) == ASE_NULL) 
		{
			mem->brooding_frame = frame->link;
			ase_lsp_freeframe (lsp, frame);
			return ASE_NULL;
		}

		actual = ASE_LSP_CDR(actual);
		formal = ASE_LSP_CDR(formal);
	}

	if (ASE_LSP_TYPE(actual) == ASE_LSP_OBJ_CONS) 
	{
		mem->brooding_frame = frame->link;
		ase_lsp_freeframe (lsp, frame);

		ase_lsp_seterror (lsp, ASE_LSP_EARGMANY, ASE_NULL, 0);
		return ASE_NULL;
	}
	else if (actual != mem->nil) 
	{
		mem->brooding_frame = frame->link;
		ase_lsp_freeframe (lsp, frame);

		ase_lsp_seterror (lsp, ASE_LSP_EARGBAD, ASE_NULL, 0);
		return ASE_NULL;
	}

	/* push the frame */
	mem->brooding_frame = frame->link;
	frame->link = mem->frame;
	mem->frame = frame;

	/* do the evaluation of the body */
	value = mem->nil;
	while (body != mem->nil) 
	{
		value = ase_lsp_eval(lsp, ASE_LSP_CAR(body));
		if (value == ASE_NULL) 
		{
			mem->frame = frame->link;
			ase_lsp_freeframe (lsp, frame);
			return ASE_NULL;
		}
		body = ASE_LSP_CDR(body);
	}

	/* pop the frame. */
	mem->frame = frame->link;

	/* destroy the frame. */
	ase_lsp_freeframe (lsp, frame);

	/*if (ASE_LSP_CAR(func) == mem->macro) {*/
	if (ASE_LSP_TYPE(func) == ASE_LSP_OBJ_MACRO) 
	{
		value = ase_lsp_eval (lsp, value);
		if (value == ASE_NULL) return ASE_NULL;
	}

	return value;
}

static ase_lsp_obj_t* apply_to_prim (
	ase_lsp_t* lsp, ase_lsp_obj_t* func, ase_lsp_obj_t* actual)
{
	ase_lsp_obj_t* obj;
	ase_size_t count = 0;

	ASE_LSP_ASSERT (lsp, ASE_LSP_TYPE(func) == ASE_LSP_OBJ_PRIM);

	obj = actual;
	while (ASE_LSP_TYPE(obj) == ASE_LSP_OBJ_CONS) 
	{
		count++;
		obj = ASE_LSP_CDR(obj);
	}	
	if (obj != lsp->mem->nil) 
	{
		ase_lsp_seterror (lsp, ASE_LSP_EARGBAD, ASE_NULL, 0);
		return ASE_NULL;
	}

	if (count < ASE_LSP_PMINARGS(func))
	{
		ase_lsp_seterror (lsp, ASE_LSP_EARGFEW, ASE_NULL, 0);
		return ASE_NULL;
	}

	if (count > ASE_LSP_PMAXARGS(func))
	{
		ase_lsp_seterror (lsp, ASE_LSP_EARGMANY, ASE_NULL, 0);
		return ASE_NULL;
	} 

	return ASE_LSP_PIMPL(func) (lsp, actual);
}
