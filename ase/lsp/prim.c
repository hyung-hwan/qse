/*
 * $Id: prim.c,v 1.2 2005-09-18 11:34:35 bacon Exp $
 */

#include <xp/lsp/lsp.h>
#include <xp/lsp/mem.h>
#include <xp/lsp/prim.h>
#include <xp/bas/assert.h>

xp_lsp_obj_t* xp_lsp_prim_abort (xp_lsp_t* lsp, xp_lsp_obj_t* args)
{
	XP_LSP_PRIM_CHECK_ARG_COUNT (lsp, args, 0, 0);
	lsp->errnum = XP_LSP_ERR_ABORT;
	return XP_NULL;
}

xp_lsp_obj_t* xp_lsp_prim_eval (xp_lsp_t* lsp, xp_lsp_obj_t* args)
{
	xp_lsp_obj_t* tmp;

	XP_LSP_PRIM_CHECK_ARG_COUNT (lsp, args, 1, 1);
	xp_assert (XP_LSP_TYPE(args) == XP_LSP_OBJ_CONS);

	tmp = xp_lsp_eval (lsp, XP_LSP_CAR(args));
	if (tmp == XP_NULL) return XP_NULL;

	tmp = xp_lsp_eval (lsp, tmp);
	if (tmp == XP_NULL) return XP_NULL;

	return tmp;
}

xp_lsp_obj_t* xp_lsp_prim_prog1 (xp_lsp_t* lsp, xp_lsp_obj_t* args)
{
	xp_lsp_obj_t* res = XP_NULL, * tmp;

	XP_LSP_PRIM_CHECK_ARG_COUNT (lsp, args, 1, XP_LSP_PRIM_MAX_ARG_COUNT);

	//while (args != lsp->mem->nil) {
	while (XP_LSP_TYPE(args) == XP_LSP_OBJ_CONS) {

		tmp = xp_lsp_eval (lsp, XP_LSP_CAR(args));
		if (tmp == XP_NULL) return XP_NULL;

		if (res == XP_NULL) {
			/*
			xp_lsp_array_t* ta = lsp->mem->temp_array;
			xp_lsp_array_insert (ta, ta->size, tmp);
			*/
			res = tmp;
		}
		args = XP_LSP_CDR(args);
	}

	return res;
}

xp_lsp_obj_t* xp_lsp_prim_progn (xp_lsp_t* lsp, xp_lsp_obj_t* args)
{
	xp_lsp_obj_t* res, * tmp;

	XP_LSP_PRIM_CHECK_ARG_COUNT (lsp, args, 1, XP_LSP_PRIM_MAX_ARG_COUNT);

	res = lsp->mem->nil;
	//while (args != lsp->mem->nil) {
	while (XP_LSP_TYPE(args) == XP_LSP_OBJ_CONS) {

		tmp = xp_lsp_eval (lsp, XP_LSP_CAR(args));
		if (tmp == XP_NULL) return XP_NULL;

		res = tmp;
		args = XP_LSP_CDR(args);
	}

	return res;
}

xp_lsp_obj_t* xp_lsp_prim_gc (xp_lsp_t* lsp, xp_lsp_obj_t* args)
{
	XP_LSP_PRIM_CHECK_ARG_COUNT (lsp, args, 0, 0);
	xp_lsp_garbage_collect (lsp->mem);
	return lsp->mem->nil;
}

xp_lsp_obj_t* xp_lsp_prim_cond (xp_lsp_t* lsp, xp_lsp_obj_t* args)
{
	/*
	 * (cond 
	 *     (condition1 result1)
	 *     (consition2 result2)
	 *     ...
	 *     (t resultN))
	 */

	xp_lsp_obj_t* tmp, * ret;

	XP_LSP_PRIM_CHECK_ARG_COUNT (lsp, args, 0, XP_LSP_PRIM_MAX_ARG_COUNT);

	while (XP_LSP_TYPE(args) == XP_LSP_OBJ_CONS) {
		if (XP_LSP_TYPE(XP_LSP_CAR(args)) != XP_LSP_OBJ_CONS) {
			lsp->errnum = XP_LSP_ERR_BAD_ARG;
			return XP_NULL;
		}

		tmp = xp_lsp_eval (lsp, XP_LSP_CAR(XP_LSP_CAR(args)));
		if (tmp == XP_NULL) return XP_NULL;

		if (tmp != lsp->mem->nil) {
			tmp = XP_LSP_CDR(XP_LSP_CAR(args));
			ret = lsp->mem->nil;
			while (XP_LSP_TYPE(tmp) == XP_LSP_OBJ_CONS) {
				ret = xp_lsp_eval (lsp, XP_LSP_CAR(tmp));
				if (ret == XP_NULL) return XP_NULL;
				tmp = XP_LSP_CDR(tmp);
			}
			if (tmp != lsp->mem->nil) {
				lsp->errnum = XP_LSP_ERR_BAD_ARG;
				return XP_NULL;
			}
			return ret;
		}

		args = XP_LSP_CDR(args);
	}

	return lsp->mem->nil;
}

xp_lsp_obj_t* xp_lsp_prim_if (xp_lsp_t* lsp, xp_lsp_obj_t* args)
{
	xp_lsp_obj_t* tmp;

	XP_LSP_PRIM_CHECK_ARG_COUNT (lsp, args, 2, XP_LSP_PRIM_MAX_ARG_COUNT);
	xp_assert (XP_LSP_TYPE(args) == XP_LSP_OBJ_CONS);
	
	tmp = xp_lsp_eval (lsp, XP_LSP_CAR(args));
	if (tmp == XP_NULL) return XP_NULL;

	if (tmp != lsp->mem->nil) {
		tmp = xp_lsp_eval (lsp, XP_LSP_CAR(XP_LSP_CDR(args)));
		if (tmp == XP_NULL) return XP_NULL;
		return tmp;
	}	
	else {
		xp_lsp_obj_t* res = lsp->mem->nil;

		tmp = XP_LSP_CDR(XP_LSP_CDR(args));

		while (XP_LSP_TYPE(tmp) == XP_LSP_OBJ_CONS) {
			res = xp_lsp_eval (lsp, XP_LSP_CAR(tmp));
			if (res == XP_NULL) return XP_NULL;
			tmp = XP_LSP_CDR(tmp);
		}
		if (tmp != lsp->mem->nil) {
			lsp->errnum = XP_LSP_ERR_BAD_ARG;
			return XP_NULL;
		}

		return res;
	}
}

xp_lsp_obj_t* xp_lsp_prim_while (xp_lsp_t* lsp, xp_lsp_obj_t* args)
{
	/*
	 * (setq a 1)
	 * (while (< a 100) (setq a (+ a 1)))
	 */

	xp_lsp_obj_t* tmp;

	XP_LSP_PRIM_CHECK_ARG_COUNT (lsp, args, 1, XP_LSP_PRIM_MAX_ARG_COUNT);
	xp_assert (XP_LSP_TYPE(args) == XP_LSP_OBJ_CONS);

	for (;;) {
		tmp = xp_lsp_eval (lsp, XP_LSP_CAR(args));
		if (tmp == XP_NULL) return XP_NULL;
		if (tmp == lsp->mem->nil) break;

		tmp = XP_LSP_CDR(args);
		while (XP_LSP_TYPE(tmp) == XP_LSP_OBJ_CONS) {
			if (xp_lsp_eval (lsp, XP_LSP_CAR(tmp)) == XP_NULL) return XP_NULL;
			tmp = XP_LSP_CDR(tmp);
		}
		if (tmp != lsp->mem->nil) {
			lsp->errnum = XP_LSP_ERR_BAD_ARG;
			return XP_NULL;
		}
	}

	return lsp->mem->nil;
}

xp_lsp_obj_t* xp_lsp_prim_car (xp_lsp_t* lsp, xp_lsp_obj_t* args)
{
	xp_lsp_obj_t* tmp;

	XP_LSP_PRIM_CHECK_ARG_COUNT (lsp, args, 1, 1);
	xp_assert (XP_LSP_TYPE(args) == XP_LSP_OBJ_CONS);

	tmp = xp_lsp_eval (lsp, XP_LSP_CAR(args));
	if (tmp == XP_NULL) return XP_NULL;
	if (tmp == lsp->mem->nil) return lsp->mem->nil;

	if (XP_LSP_TYPE(tmp) != XP_LSP_OBJ_CONS) {
		lsp->errnum = XP_LSP_ERR_BAD_ARG;
		return XP_NULL;
	}

	return XP_LSP_CAR(tmp);
}

xp_lsp_obj_t* xp_lsp_prim_cdr (xp_lsp_t* lsp, xp_lsp_obj_t* args)
{
	xp_lsp_obj_t* tmp;

	XP_LSP_PRIM_CHECK_ARG_COUNT (lsp, args, 1, 1);
	xp_assert (XP_LSP_TYPE(args) == XP_LSP_OBJ_CONS);

	tmp = xp_lsp_eval (lsp, XP_LSP_CAR(args));
	if (tmp == XP_NULL) return XP_NULL;
	if (tmp == lsp->mem->nil) return lsp->mem->nil;

	if (XP_LSP_TYPE(tmp) != XP_LSP_OBJ_CONS) {
		lsp->errnum = XP_LSP_ERR_BAD_ARG;
		return XP_NULL;
	}

	return XP_LSP_CDR(tmp);
}

xp_lsp_obj_t* xp_lsp_prim_cons (xp_lsp_t* lsp, xp_lsp_obj_t* args)
{
	xp_lsp_obj_t* car, * cdr, * cons;

	XP_LSP_PRIM_CHECK_ARG_COUNT (lsp, args, 2, 2);
	xp_assert (XP_LSP_TYPE(args) == XP_LSP_OBJ_CONS);

	car = xp_lsp_eval (lsp, XP_LSP_CAR(args));
	if (car == XP_NULL) return XP_NULL;

	cdr = xp_lsp_eval (lsp, XP_LSP_CAR(XP_LSP_CDR(args)));
	if (cdr == XP_NULL) return XP_NULL;

	cons = xp_lsp_make_cons (lsp->mem, car, cdr);
	if (cons == XP_NULL) {
		lsp->errnum = XP_LSP_ERR_MEM;
		return XP_NULL;
	}

	return cons;
}

xp_lsp_obj_t* xp_lsp_prim_set (xp_lsp_t* lsp, xp_lsp_obj_t* args)
{
	xp_lsp_obj_t* p1, * p2;

	XP_LSP_PRIM_CHECK_ARG_COUNT (lsp, args, 2, 2);
	xp_assert (XP_LSP_TYPE(args) == XP_LSP_OBJ_CONS);

	p1 = xp_lsp_eval (lsp, XP_LSP_CAR(args));
	if (p1 == XP_NULL) return XP_NULL;

	if (XP_LSP_TYPE(p1) != XP_LSP_OBJ_SYMBOL) {
		lsp->errnum = XP_LSP_ERR_BAD_ARG;
		return XP_NULL;
	}

	p2 = xp_lsp_eval (lsp, XP_LSP_CAR(XP_LSP_CDR(args)));
	if (p2 == XP_NULL) return XP_NULL;

	if (xp_lsp_set (lsp->mem, p1, p2) == XP_NULL) {
		lsp->errnum = XP_LSP_ERR_MEM;
		return XP_NULL;
	}

	return p2;
}

xp_lsp_obj_t* xp_lsp_prim_setq (xp_lsp_t* lsp, xp_lsp_obj_t* args)
{
	xp_lsp_obj_t* p = args, * p1, * p2 = lsp->mem->nil;

	while (p != lsp->mem->nil) {
		xp_assert (XP_LSP_TYPE(p) == XP_LSP_OBJ_CONS);

		p1 = XP_LSP_CAR(p);
		if (XP_LSP_TYPE(p1) != XP_LSP_OBJ_SYMBOL) {
			lsp->errnum = XP_LSP_ERR_BAD_ARG;
			return XP_NULL;
		}

		if (XP_LSP_TYPE(XP_LSP_CDR(p)) != XP_LSP_OBJ_CONS) {
			lsp->errnum = XP_LSP_ERR_TOO_FEW_ARGS;
			return XP_NULL;
		}

		p2 = xp_lsp_eval (lsp, XP_LSP_CAR(XP_LSP_CDR(p)));
		if (p2 == XP_NULL) return XP_NULL;

		if (xp_lsp_set (lsp->mem, p1, p2) == XP_NULL) {
			lsp->errnum = XP_LSP_ERR_MEM;
			return XP_NULL;
		}

		p = XP_LSP_CDR(XP_LSP_CDR(p));
	}

	return p2;
}

xp_lsp_obj_t* xp_lsp_prim_quote (xp_lsp_t* lsp, xp_lsp_obj_t* args)
{
	XP_LSP_PRIM_CHECK_ARG_COUNT (lsp, args, 1, 1);
	xp_assert (XP_LSP_TYPE(args) == XP_LSP_OBJ_CONS);
	return XP_LSP_CAR(args);
}

xp_lsp_obj_t* xp_lsp_prim_defun (xp_lsp_t* lsp, xp_lsp_obj_t* args)
{
	/*
	 * (defun x (abc)  x y z)
	 * (setq x (lambda (abc) x y z))
	 */

	xp_lsp_obj_t* name, * fun;

	XP_LSP_PRIM_CHECK_ARG_COUNT (lsp, args, 3, XP_LSP_PRIM_MAX_ARG_COUNT);

	name = XP_LSP_CAR(args);
	if (XP_LSP_TYPE(name) != XP_LSP_OBJ_SYMBOL) {
		lsp->errnum = XP_LSP_ERR_BAD_ARG;
		return XP_NULL;
	}

	fun = xp_lsp_make_func (lsp->mem, 
		XP_LSP_CAR(XP_LSP_CDR(args)), XP_LSP_CDR(XP_LSP_CDR(args)));
	if (fun == XP_NULL) return XP_NULL;

	if (xp_lsp_set (lsp->mem, XP_LSP_CAR(args), fun) == XP_NULL) {
		lsp->errnum = XP_LSP_ERR_MEM;
		return XP_NULL;
	}
	return fun;
}

xp_lsp_obj_t* xp_lsp_prim_demac (xp_lsp_t* lsp, xp_lsp_obj_t* args)
{
	/*
	 * (demac x (abc)  x y z)
	 *(setq x (macro (abc) x y z))
	 */

	xp_lsp_obj_t* name, * mac;

	XP_LSP_PRIM_CHECK_ARG_COUNT (lsp, args, 3, XP_LSP_PRIM_MAX_ARG_COUNT);

	name = XP_LSP_CAR(args);
	if (XP_LSP_TYPE(name) != XP_LSP_OBJ_SYMBOL) {
		lsp->errnum = XP_LSP_ERR_BAD_ARG;
		return XP_NULL;
	}

	mac = xp_lsp_make_macro (lsp->mem, 
		XP_LSP_CAR(XP_LSP_CDR(args)), XP_LSP_CDR(XP_LSP_CDR(args)));
	if (mac == XP_NULL) return XP_NULL;

	if (xp_lsp_set (lsp->mem, XP_LSP_CAR(args), mac) == XP_NULL) {
		lsp->errnum = XP_LSP_ERR_MEM;
		return XP_NULL;
	}
	return mac;
}

static xp_lsp_obj_t* xp_lsp_prim_let_impl (
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
		lsp->errnum = XP_LSP_ERR_MEM;
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
			if (xp_lsp_frame_insert (frame, n, v) == XP_NULL) {
				lsp->errnum = XP_LSP_ERR_MEM;
				if (sequential) lsp->mem->frame = frame->link;
				else lsp->mem->brooding_frame = frame->link;
				xp_lsp_frame_free (frame);
				return XP_NULL;
			}
		}
		else if (XP_LSP_TYPE(ass) == XP_LSP_OBJ_SYMBOL) {
			if (xp_lsp_frame_lookup (frame, ass) != XP_NULL) {
				lsp->errnum = XP_LSP_ERR_DUP_FORMAL;
				if (sequential) lsp->mem->frame = frame->link;
				else lsp->mem->brooding_frame = frame->link;
				xp_lsp_frame_free (frame);
				return XP_NULL;
			}
			if (xp_lsp_frame_insert (frame, ass, lsp->mem->nil) == XP_NULL) {
				lsp->errnum = XP_LSP_ERR_MEM;
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
	return xp_lsp_prim_let_impl (lsp, args, 0);
}

xp_lsp_obj_t* xp_lsp_prim_letx (xp_lsp_t* lsp, xp_lsp_obj_t* args)
{
	return xp_lsp_prim_let_impl (lsp, args, 1);
}

xp_lsp_obj_t* xp_lsp_prim_plus (xp_lsp_t* lsp, xp_lsp_obj_t* args)
{
	xp_lsp_obj_t* body, * tmp;
	xp_lsp_int_t value = 0;

	XP_LSP_PRIM_CHECK_ARG_COUNT (lsp, args, 1, XP_LSP_PRIM_MAX_ARG_COUNT);
	xp_assert (XP_LSP_TYPE(args) == XP_LSP_OBJ_CONS);

	body = args;
	//while (body != lsp->mem->nil) {
	while (XP_LSP_TYPE(body) == XP_LSP_OBJ_CONS) {
		tmp = xp_lsp_eval (lsp, XP_LSP_CAR(body));
		if (tmp == XP_NULL) return XP_NULL;

		if (XP_LSP_TYPE(tmp) != XP_LSP_OBJ_INT) {
			lsp->errnum = XP_LSP_ERR_BAD_VALUE;	
			return XP_NULL;
		}

		value = value + XP_LSP_IVALUE(tmp);
		body = XP_LSP_CDR(body);
	}

	tmp = xp_lsp_make_int (lsp->mem, value);
	if (tmp == XP_NULL) {
		lsp->errnum = XP_LSP_ERR_MEM;
		return XP_NULL;
	}

	return tmp;
}

xp_lsp_obj_t* xp_lsp_prim_gt (xp_lsp_t* lsp, xp_lsp_obj_t* args)
{
	xp_lsp_obj_t* p1, * p2;
	int res;

	XP_LSP_PRIM_CHECK_ARG_COUNT (lsp, args, 2, 2);
	xp_assert (XP_LSP_TYPE(args) == XP_LSP_OBJ_CONS);

	p1 = xp_lsp_eval (lsp, XP_LSP_CAR(args));
	if (p1 == XP_NULL) return XP_NULL;
	// TODO: lock p1....

	p2 = xp_lsp_eval (lsp, XP_LSP_CAR(XP_LSP_CDR(args)));
	if (p2 == XP_NULL) return XP_NULL;

	if (XP_LSP_TYPE(p1) == XP_LSP_OBJ_INT) {
		if (XP_LSP_TYPE(p2) == XP_LSP_OBJ_INT) {
			res = XP_LSP_IVALUE(p1) > XP_LSP_IVALUE(p2);
		}
		else if (XP_LSP_TYPE(p2) == XP_LSP_OBJ_FLOAT) {
			res = XP_LSP_IVALUE(p1) > XP_LSP_FVALUE(p2);
		}
		else {
			lsp->errnum = XP_LSP_ERR_BAD_VALUE;
			return XP_NULL;
		}
	}
	else if (XP_LSP_TYPE(p1) == XP_LSP_OBJ_FLOAT) {
		if (XP_LSP_TYPE(p2) == XP_LSP_OBJ_INT) {
			res = XP_LSP_FVALUE(p1) > XP_LSP_IVALUE(p2);
		}
		else if (XP_LSP_TYPE(p2) == XP_LSP_OBJ_FLOAT) {
			res = XP_LSP_FVALUE(p1) > XP_LSP_FVALUE(p2);
		}
		else {
			lsp->errnum = XP_LSP_ERR_BAD_VALUE;
			return XP_NULL;
		}
	}
	else if (XP_LSP_TYPE(p1) == XP_LSP_OBJ_SYMBOL) {
		if (XP_LSP_TYPE(p2) == XP_LSP_OBJ_SYMBOL) {
			res = xp_lsp_comp_symbol2 (
				p1, XP_LSP_SYMVALUE(p2), XP_LSP_SYMLEN(p2)) > 0;
		}
		else {
			lsp->errnum = XP_LSP_ERR_BAD_VALUE;
			return XP_NULL;
		}
	}
	else if (XP_LSP_TYPE(p1) == XP_LSP_OBJ_STRING) {
		if (XP_LSP_TYPE(p2) == XP_LSP_OBJ_STRING) {
			res = xp_lsp_comp_string2 (
				p1, XP_LSP_STRVALUE(p2), XP_LSP_STRLEN(p2)) > 0;
		}
		else {
			lsp->errnum = XP_LSP_ERR_BAD_VALUE;
			return XP_NULL;
		}
	}
	else {
		lsp->errnum = XP_LSP_ERR_BAD_VALUE;
		return XP_NULL;
	}

	return (res)? lsp->mem->t: lsp->mem->nil;
}

xp_lsp_obj_t* xp_lsp_prim_lt (xp_lsp_t* lsp, xp_lsp_obj_t* args)
{
	xp_lsp_obj_t* p1, * p2;
	int res;

	XP_LSP_PRIM_CHECK_ARG_COUNT (lsp, args, 2, 2);
	xp_assert (XP_LSP_TYPE(args) == XP_LSP_OBJ_CONS);

	p1 = xp_lsp_eval (lsp, XP_LSP_CAR(args));
	if (p1 == XP_NULL) return XP_NULL;
	// TODO: lock p1....

	p2 = xp_lsp_eval (lsp, XP_LSP_CAR(XP_LSP_CDR(args)));
	if (p2 == XP_NULL) return XP_NULL;

	if (XP_LSP_TYPE(p1) == XP_LSP_OBJ_INT) {
		if (XP_LSP_TYPE(p2) == XP_LSP_OBJ_INT) {
			res = XP_LSP_IVALUE(p1) < XP_LSP_IVALUE(p2);
		}
		else if (XP_LSP_TYPE(p2) == XP_LSP_OBJ_FLOAT) {
			res = XP_LSP_IVALUE(p1) < XP_LSP_FVALUE(p2);
		}
		else {
			lsp->errnum = XP_LSP_ERR_BAD_VALUE;
			return XP_NULL;
		}
	}
	else if (XP_LSP_TYPE(p1) == XP_LSP_OBJ_FLOAT) {
		if (XP_LSP_TYPE(p2) == XP_LSP_OBJ_INT) {
			res = XP_LSP_FVALUE(p1) < XP_LSP_IVALUE(p2);
		}
		else if (XP_LSP_TYPE(p2) == XP_LSP_OBJ_FLOAT) {
			res = XP_LSP_FVALUE(p1) < XP_LSP_FVALUE(p2);
		}
		else {
			lsp->errnum = XP_LSP_ERR_BAD_VALUE;
			return XP_NULL;
		}
	}
	else if (XP_LSP_TYPE(p1) == XP_LSP_OBJ_SYMBOL) {
		if (XP_LSP_TYPE(p2) == XP_LSP_OBJ_SYMBOL) {
			res = xp_lsp_comp_symbol2 (
				p1, XP_LSP_SYMVALUE(p2), XP_LSP_SYMLEN(p2)) < 0;
		}
		else {
			lsp->errnum = XP_LSP_ERR_BAD_VALUE;
			return XP_NULL;
		}
	}
	else if (XP_LSP_TYPE(p1) == XP_LSP_OBJ_STRING) {
		if (XP_LSP_TYPE(p2) == XP_LSP_OBJ_STRING) {
			res = xp_lsp_comp_string2 (
				p1, XP_LSP_STRVALUE(p2), XP_LSP_STRLEN(p2)) < 0;
		}
		else {
			lsp->errnum = XP_LSP_ERR_BAD_VALUE;
			return XP_NULL;
		}
	}
	else {
		lsp->errnum = XP_LSP_ERR_BAD_VALUE;
		return XP_NULL;
	}

	return (res)? lsp->mem->t: lsp->mem->nil;
}
