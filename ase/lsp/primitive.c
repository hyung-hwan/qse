/*
 * $Id: primitive.c,v 1.3 2005-02-05 05:18:20 bacon Exp $
 */

#include <xp/lisp/lisp.h>
#include <xp/lisp/memory.h>
#include <xp/lisp/primitive.h>

xp_lisp_obj_t* xp_lisp_prim_abort (xp_lisp_t* lsp, xp_lisp_obj_t* args)
{
	XP_LISP_PRIM_CHECK_ARG_COUNT (lsp, args, 0, 0);
	lsp->error = XP_LISP_ERR_ABORT;
	return XP_NULL;
}

xp_lisp_obj_t* xp_lisp_prim_eval (xp_lisp_t* lsp, xp_lisp_obj_t* args)
{
	xp_lisp_obj_t* tmp;

	XP_LISP_PRIM_CHECK_ARG_COUNT (lsp, args, 1, 1);
	xp_assert (XP_LISP_TYPE(args) == XP_LISP_OBJ_CONS);

	tmp = xp_lisp_eval (lsp, XP_LISP_CAR(args));
	if (tmp == XP_NULL) return XP_NULL;

	tmp = xp_lisp_eval (lsp, tmp);
	if (tmp == XP_NULL) return XP_NULL;

	return tmp;
}

xp_lisp_obj_t* xp_lisp_prim_prog1 (xp_lisp_t* lsp, xp_lisp_obj_t* args)
{
	xp_lisp_obj_t* res = XP_NULL, * tmp;

	XP_LISP_PRIM_CHECK_ARG_COUNT (lsp, args, 1, XP_LISP_PRIM_MAX_ARG_COUNT);

	//while (args != lsp->mem->nil) {
	while (XP_LISP_TYPE(args) == XP_LISP_OBJ_CONS) {

		tmp = xp_lisp_eval (lsp, XP_LISP_CAR(args));
		if (tmp == XP_NULL) return XP_NULL;

		if (res == XP_NULL) {
			/*
			xp_lisp_array_t* ta = lsp->mem->temp_array;
			xp_lisp_array_insert (ta, ta->size, tmp);
			*/
			res = tmp;
		}
		args = XP_LISP_CDR(args);
	}

	return res;
}

xp_lisp_obj_t* xp_lisp_prim_progn (xp_lisp_t* lsp, xp_lisp_obj_t* args)
{
	xp_lisp_obj_t* res, * tmp;

	XP_LISP_PRIM_CHECK_ARG_COUNT (lsp, args, 1, XP_LISP_PRIM_MAX_ARG_COUNT);

	res = lsp->mem->nil;
	//while (args != lsp->mem->nil) {
	while (XP_LISP_TYPE(args) == XP_LISP_OBJ_CONS) {

		tmp = xp_lisp_eval (lsp, XP_LISP_CAR(args));
		if (tmp == XP_NULL) return XP_NULL;

		res = tmp;
		args = XP_LISP_CDR(args);
	}

	return res;
}

xp_lisp_obj_t* xp_lisp_prim_gc (xp_lisp_t* lsp, xp_lisp_obj_t* args)
{
	XP_LISP_PRIM_CHECK_ARG_COUNT (lsp, args, 0, 0);
	xp_lisp_garbage_collect (lsp->mem);
	return lsp->mem->nil;
}

xp_lisp_obj_t* xp_lisp_prim_cond (xp_lisp_t* lsp, xp_lisp_obj_t* args)
{
	/*
	 * (cond 
	 *     (condition1 result1)
	 *     (consition2 result2)
	 *     ...
	 *     (t resultN))
	 */

	xp_lisp_obj_t* tmp, * ret;

	XP_LISP_PRIM_CHECK_ARG_COUNT (lsp, args, 0, XP_LISP_PRIM_MAX_ARG_COUNT);

	while (XP_LISP_TYPE(args) == XP_LISP_OBJ_CONS) {
		if (XP_LISP_TYPE(XP_LISP_CAR(args)) != XP_LISP_OBJ_CONS) {
			lsp->error = XP_LISP_ERR_BAD_ARG;
			return XP_NULL;
		}

		tmp = xp_lisp_eval (lsp, XP_LISP_CAR(XP_LISP_CAR(args)));
		if (tmp == XP_NULL) return XP_NULL;

		if (tmp != lsp->mem->nil) {
			tmp = XP_LISP_CDR(XP_LISP_CAR(args));
			ret = lsp->mem->nil;
			while (XP_LISP_TYPE(tmp) == XP_LISP_OBJ_CONS) {
				ret = xp_lisp_eval (lsp, XP_LISP_CAR(tmp));
				if (ret == XP_NULL) return XP_NULL;
				tmp = XP_LISP_CDR(tmp);
			}
			if (tmp != lsp->mem->nil) {
				lsp->error = XP_LISP_ERR_BAD_ARG;
				return XP_NULL;
			}
			return ret;
		}

		args = XP_LISP_CDR(args);
	}

	return lsp->mem->nil;
}

xp_lisp_obj_t* xp_lisp_prim_if (xp_lisp_t* lsp, xp_lisp_obj_t* args)
{
	xp_lisp_obj_t* tmp;

	XP_LISP_PRIM_CHECK_ARG_COUNT (lsp, args, 2, XP_LISP_PRIM_MAX_ARG_COUNT);
	xp_assert (XP_LISP_TYPE(args) == XP_LISP_OBJ_CONS);
	
	tmp = xp_lisp_eval (lsp, XP_LISP_CAR(args));
	if (tmp == XP_NULL) return XP_NULL;

	if (tmp != lsp->mem->nil) {
		tmp = xp_lisp_eval (lsp, XP_LISP_CAR(XP_LISP_CDR(args)));
		if (tmp == XP_NULL) return XP_NULL;
		return tmp;
	}	
	else {
		xp_lisp_obj_t* res = lsp->mem->nil;

		tmp = XP_LISP_CDR(XP_LISP_CDR(args));

		while (XP_LISP_TYPE(tmp) == XP_LISP_OBJ_CONS) {
			res = xp_lisp_eval (lsp, XP_LISP_CAR(tmp));
			if (res == XP_NULL) return XP_NULL;
			tmp = XP_LISP_CDR(tmp);
		}
		if (tmp != lsp->mem->nil) {
			lsp->error = XP_LISP_ERR_BAD_ARG;
			return XP_NULL;
		}

		return res;
	}
}

xp_lisp_obj_t* xp_lisp_prim_while (xp_lisp_t* lsp, xp_lisp_obj_t* args)
{
	/*
	 * (setq a 1)
	 * (while (< a 100) (setq a (+ a 1)))
	 */

	xp_lisp_obj_t* tmp;

	XP_LISP_PRIM_CHECK_ARG_COUNT (lsp, args, 1, XP_LISP_PRIM_MAX_ARG_COUNT);
	xp_assert (XP_LISP_TYPE(args) == XP_LISP_OBJ_CONS);

	for (;;) {
		tmp = xp_lisp_eval (lsp, XP_LISP_CAR(args));
		if (tmp == XP_NULL) return XP_NULL;
		if (tmp == lsp->mem->nil) break;

		tmp = XP_LISP_CDR(args);
		while (XP_LISP_TYPE(tmp) == XP_LISP_OBJ_CONS) {
			if (xp_lisp_eval (lsp, XP_LISP_CAR(tmp)) == XP_NULL) return XP_NULL;
			tmp = XP_LISP_CDR(tmp);
		}
		if (tmp != lsp->mem->nil) {
			lsp->error = XP_LISP_ERR_BAD_ARG;
			return XP_NULL;
		}
	}

	return lsp->mem->nil;
}

xp_lisp_obj_t* xp_lisp_prim_car (xp_lisp_t* lsp, xp_lisp_obj_t* args)
{
	xp_lisp_obj_t* tmp;

	XP_LISP_PRIM_CHECK_ARG_COUNT (lsp, args, 1, 1);
	xp_assert (XP_LISP_TYPE(args) == XP_LISP_OBJ_CONS);

	tmp = xp_lisp_eval (lsp, XP_LISP_CAR(args));
	if (tmp == XP_NULL) return XP_NULL;
	if (tmp == lsp->mem->nil) return lsp->mem->nil;

	if (XP_LISP_TYPE(tmp) != XP_LISP_OBJ_CONS) {
		lsp->error = XP_LISP_ERR_BAD_ARG;
		return XP_NULL;
	}

	return XP_LISP_CAR(tmp);
}

xp_lisp_obj_t* xp_lisp_prim_cdr (xp_lisp_t* lsp, xp_lisp_obj_t* args)
{
	xp_lisp_obj_t* tmp;

	XP_LISP_PRIM_CHECK_ARG_COUNT (lsp, args, 1, 1);
	xp_assert (XP_LISP_TYPE(args) == XP_LISP_OBJ_CONS);

	tmp = xp_lisp_eval (lsp, XP_LISP_CAR(args));
	if (tmp == XP_NULL) return XP_NULL;
	if (tmp == lsp->mem->nil) return lsp->mem->nil;

	if (XP_LISP_TYPE(tmp) != XP_LISP_OBJ_CONS) {
		lsp->error = XP_LISP_ERR_BAD_ARG;
		return XP_NULL;
	}

	return XP_LISP_CDR(tmp);
}

xp_lisp_obj_t* xp_lisp_prim_cons (xp_lisp_t* lsp, xp_lisp_obj_t* args)
{
	xp_lisp_obj_t* car, * cdr, * cons;

	XP_LISP_PRIM_CHECK_ARG_COUNT (lsp, args, 2, 2);
	xp_assert (XP_LISP_TYPE(args) == XP_LISP_OBJ_CONS);

	car = xp_lisp_eval (lsp, XP_LISP_CAR(args));
	if (car == XP_NULL) return XP_NULL;

	cdr = xp_lisp_eval (lsp, XP_LISP_CAR(XP_LISP_CDR(args)));
	if (cdr == XP_NULL) return XP_NULL;

	cons = xp_lisp_make_cons (lsp->mem, car, cdr);
	if (cons == XP_NULL) {
		lsp->error = XP_LISP_ERR_MEM;
		return XP_NULL;
	}

	return cons;
}

xp_lisp_obj_t* xp_lisp_prim_set (xp_lisp_t* lsp, xp_lisp_obj_t* args)
{
	xp_lisp_obj_t* p1, * p2;

	XP_LISP_PRIM_CHECK_ARG_COUNT (lsp, args, 2, 2);
	xp_assert (XP_LISP_TYPE(args) == XP_LISP_OBJ_CONS);

	p1 = xp_lisp_eval (lsp, XP_LISP_CAR(args));
	if (p1 == XP_NULL) return XP_NULL;

	if (XP_LISP_TYPE(p1) != XP_LISP_OBJ_SYMBOL) {
		lsp->error = XP_LISP_ERR_BAD_ARG;
		return XP_NULL;
	}

	p2 = xp_lisp_eval (lsp, XP_LISP_CAR(XP_LISP_CDR(args)));
	if (p2 == XP_NULL) return XP_NULL;

	if (xp_lisp_set (lsp->mem, p1, p2) == XP_NULL) {
		lsp->error = XP_LISP_ERR_MEM;
		return XP_NULL;
	}

	return p2;
}

xp_lisp_obj_t* xp_lisp_prim_setq (xp_lisp_t* lsp, xp_lisp_obj_t* args)
{
	xp_lisp_obj_t* p = args, * p1, * p2 = lsp->mem->nil;

	while (p != lsp->mem->nil) {
		xp_assert (XP_LISP_TYPE(p) == XP_LISP_OBJ_CONS);

		p1 = XP_LISP_CAR(p);
		if (XP_LISP_TYPE(p1) != XP_LISP_OBJ_SYMBOL) {
			lsp->error = XP_LISP_ERR_BAD_ARG;
			return XP_NULL;
		}

		if (XP_LISP_TYPE(XP_LISP_CDR(p)) != XP_LISP_OBJ_CONS) {
			lsp->error = XP_LISP_ERR_TOO_FEW_ARGS;
			return XP_NULL;
		}

		p2 = xp_lisp_eval (lsp, XP_LISP_CAR(XP_LISP_CDR(p)));
		if (p2 == XP_NULL) return XP_NULL;

		if (xp_lisp_set (lsp->mem, p1, p2) == XP_NULL) {
			lsp->error = XP_LISP_ERR_MEM;
			return XP_NULL;
		}

		p = XP_LISP_CDR(XP_LISP_CDR(p));
	}

	return p2;
}

xp_lisp_obj_t* xp_lisp_prim_quote (xp_lisp_t* lsp, xp_lisp_obj_t* args)
{
	XP_LISP_PRIM_CHECK_ARG_COUNT (lsp, args, 1, 1);
	xp_assert (XP_LISP_TYPE(args) == XP_LISP_OBJ_CONS);
	return XP_LISP_CAR(args);
}

xp_lisp_obj_t* xp_lisp_prim_defun (xp_lisp_t* lsp, xp_lisp_obj_t* args)
{
	/*
	 * (defun x (abc)  x y z)
	 * (setq x (lambda (abc) x y z))
	 */

	xp_lisp_obj_t* name, * fun;

	XP_LISP_PRIM_CHECK_ARG_COUNT (lsp, args, 3, XP_LISP_PRIM_MAX_ARG_COUNT);

	name = XP_LISP_CAR(args);
	if (XP_LISP_TYPE(name) != XP_LISP_OBJ_SYMBOL) {
		lsp->error = XP_LISP_ERR_BAD_ARG;
		return XP_NULL;
	}

	fun = xp_lisp_make_func (lsp->mem, 
		XP_LISP_CAR(XP_LISP_CDR(args)), XP_LISP_CDR(XP_LISP_CDR(args)));
	if (fun == XP_NULL) return XP_NULL;

	if (xp_lisp_set (lsp->mem, XP_LISP_CAR(args), fun) == XP_NULL) {
		lsp->error = XP_LISP_ERR_MEM;
		return XP_NULL;
	}
	return fun;
}

xp_lisp_obj_t* xp_lisp_prim_demac (xp_lisp_t* lsp, xp_lisp_obj_t* args)
{
	/*
	 * (demac x (abc)  x y z)
	 *(setq x (macro (abc) x y z))
	 */

	xp_lisp_obj_t* name, * mac;

	XP_LISP_PRIM_CHECK_ARG_COUNT (lsp, args, 3, XP_LISP_PRIM_MAX_ARG_COUNT);

	name = XP_LISP_CAR(args);
	if (XP_LISP_TYPE(name) != XP_LISP_OBJ_SYMBOL) {
		lsp->error = XP_LISP_ERR_BAD_ARG;
		return XP_NULL;
	}

	mac = xp_lisp_make_macro (lsp->mem, 
		XP_LISP_CAR(XP_LISP_CDR(args)), XP_LISP_CDR(XP_LISP_CDR(args)));
	if (mac == XP_NULL) return XP_NULL;

	if (xp_lisp_set (lsp->mem, XP_LISP_CAR(args), mac) == XP_NULL) {
		lsp->error = XP_LISP_ERR_MEM;
		return XP_NULL;
	}
	return mac;
}

static xp_lisp_obj_t* xp_lisp_prim_let_impl (
	xp_lisp_t* lsp, xp_lisp_obj_t* args, int sequential)
{
	xp_lisp_frame_t* frame;
	xp_lisp_obj_t* assoc;
	xp_lisp_obj_t* body;
	xp_lisp_obj_t* value;

	XP_LISP_PRIM_CHECK_ARG_COUNT (lsp, args, 1, XP_LISP_PRIM_MAX_ARG_COUNT);

	// create a new frame
	frame = xp_lisp_frame_new ();
	if (frame == XP_NULL) {
		lsp->error = XP_LISP_ERR_MEM;
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

	assoc = XP_LISP_CAR(args);

	//while (assoc != lsp->mem->nil) {
	while (XP_LISP_TYPE(assoc) == XP_LISP_OBJ_CONS) {
		xp_lisp_obj_t* ass = XP_LISP_CAR(assoc);
		if (XP_LISP_TYPE(ass) == XP_LISP_OBJ_CONS) {
			xp_lisp_obj_t* n = XP_LISP_CAR(ass);
			xp_lisp_obj_t* v = XP_LISP_CDR(ass);

			if (XP_LISP_TYPE(n) != XP_LISP_OBJ_SYMBOL) {
				lsp->error = XP_LISP_ERR_BAD_ARG; // must be a symbol
				if (sequential) lsp->mem->frame = frame->link;
				else lsp->mem->brooding_frame = frame->link;
				xp_lisp_frame_free (frame);
				return XP_NULL;
			}

			if (v != lsp->mem->nil) {
				if (XP_LISP_CDR(v) != lsp->mem->nil) {
					lsp->error = XP_LISP_ERR_TOO_MANY_ARGS; // must be a symbol
					if (sequential) lsp->mem->frame = frame->link;
					else lsp->mem->brooding_frame = frame->link;
					xp_lisp_frame_free (frame);
					return XP_NULL;
				}
				if ((v = xp_lisp_eval(lsp, XP_LISP_CAR(v))) == XP_NULL) {
					if (sequential) lsp->mem->frame = frame->link;
					else lsp->mem->brooding_frame = frame->link;
					xp_lisp_frame_free (frame);
					return XP_NULL;
				}
			}

			if (xp_lisp_frame_lookup (frame, n) != XP_NULL) {
				lsp->error = XP_LISP_ERR_DUP_FORMAL;
				if (sequential) lsp->mem->frame = frame->link;
				else lsp->mem->brooding_frame = frame->link;
				xp_lisp_frame_free (frame);
				return XP_NULL;
			}
			if (xp_lisp_frame_insert (frame, n, v) == XP_NULL) {
				lsp->error = XP_LISP_ERR_MEM;
				if (sequential) lsp->mem->frame = frame->link;
				else lsp->mem->brooding_frame = frame->link;
				xp_lisp_frame_free (frame);
				return XP_NULL;
			}
		}
		else if (XP_LISP_TYPE(ass) == XP_LISP_OBJ_SYMBOL) {
			if (xp_lisp_frame_lookup (frame, ass) != XP_NULL) {
				lsp->error = XP_LISP_ERR_DUP_FORMAL;
				if (sequential) lsp->mem->frame = frame->link;
				else lsp->mem->brooding_frame = frame->link;
				xp_lisp_frame_free (frame);
				return XP_NULL;
			}
			if (xp_lisp_frame_insert (frame, ass, lsp->mem->nil) == XP_NULL) {
				lsp->error = XP_LISP_ERR_MEM;
				if (sequential) lsp->mem->frame = frame->link;
				else lsp->mem->brooding_frame = frame->link;
				xp_lisp_frame_free (frame);
				return XP_NULL;
			}
		}
		else {
			lsp->error = XP_LISP_ERR_BAD_ARG;		
			if (sequential) lsp->mem->frame = frame->link;
			else lsp->mem->brooding_frame = frame->link;
			xp_lisp_frame_free (frame);
			return XP_NULL;
		}

		assoc = XP_LISP_CDR(assoc);
	}

	if (assoc != lsp->mem->nil) {
		lsp->error = XP_LISP_ERR_BAD_ARG;	
		if (sequential) lsp->mem->frame = frame->link;
		else lsp->mem->brooding_frame = frame->link;
		xp_lisp_frame_free (frame);
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
	body = XP_LISP_CDR(args);
	while (body != lsp->mem->nil) {
		value = xp_lisp_eval (lsp, XP_LISP_CAR(body));
		if (value == XP_NULL) {
			lsp->mem->frame = frame->link;
			xp_lisp_frame_free (frame);
			return XP_NULL;
		}
		body = XP_LISP_CDR(body);
	}

	// pop the frame
	lsp->mem->frame = frame->link;

	// destroy the frame
	xp_lisp_frame_free (frame);
	return value;
}

xp_lisp_obj_t* xp_lisp_prim_let (xp_lisp_t* lsp, xp_lisp_obj_t* args)
{
	return xp_lisp_prim_let_impl (lsp, args, 0);
}

xp_lisp_obj_t* xp_lisp_prim_letx (xp_lisp_t* lsp, xp_lisp_obj_t* args)
{
	return xp_lisp_prim_let_impl (lsp, args, 1);
}

xp_lisp_obj_t* xp_lisp_prim_plus (xp_lisp_t* lsp, xp_lisp_obj_t* args)
{
	xp_lisp_obj_t* body, * tmp;
	xp_lisp_int value = 0;

	XP_LISP_PRIM_CHECK_ARG_COUNT (lsp, args, 1, XP_LISP_PRIM_MAX_ARG_COUNT);
	xp_assert (XP_LISP_TYPE(args) == XP_LISP_OBJ_CONS);

	body = args;
	//while (body != lsp->mem->nil) {
	while (XP_LISP_TYPE(body) == XP_LISP_OBJ_CONS) {
		tmp = xp_lisp_eval (lsp, XP_LISP_CAR(body));
		if (tmp == XP_NULL) return XP_NULL;

		if (XP_LISP_TYPE(tmp) != XP_LISP_OBJ_INT) {
			lsp->error = XP_LISP_ERR_BAD_VALUE;	
			return XP_NULL;
		}

		value = value + XP_LISP_IVALUE(tmp);
		body = XP_LISP_CDR(body);
	}

	tmp = xp_lisp_make_int (lsp->mem, value);
	if (tmp == XP_NULL) {
		lsp->error = XP_LISP_ERR_MEM;
		return XP_NULL;
	}

	return tmp;
}

xp_lisp_obj_t* xp_lisp_prim_gt (xp_lisp_t* lsp, xp_lisp_obj_t* args)
{
	xp_lisp_obj_t* p1, * p2;
	int res;

	XP_LISP_PRIM_CHECK_ARG_COUNT (lsp, args, 2, 2);
	xp_assert (XP_LISP_TYPE(args) == XP_LISP_OBJ_CONS);

	p1 = xp_lisp_eval (lsp, XP_LISP_CAR(args));
	if (p1 == XP_NULL) return XP_NULL;
	// TODO: lock p1....

	p2 = xp_lisp_eval (lsp, XP_LISP_CAR(XP_LISP_CDR(args)));
	if (p2 == XP_NULL) return XP_NULL;

	if (XP_LISP_TYPE(p1) == XP_LISP_OBJ_INT) {
		if (XP_LISP_TYPE(p2) == XP_LISP_OBJ_INT) {
			res = XP_LISP_IVALUE(p1) > XP_LISP_IVALUE(p2);
		}
		else if (XP_LISP_TYPE(p2) == XP_LISP_OBJ_FLOAT) {
			res = XP_LISP_IVALUE(p1) > XP_LISP_FVALUE(p2);
		}
		else {
			lsp->error = XP_LISP_ERR_BAD_VALUE;
			return XP_NULL;
		}
	}
	else if (XP_LISP_TYPE(p1) == XP_LISP_OBJ_FLOAT) {
		if (XP_LISP_TYPE(p2) == XP_LISP_OBJ_INT) {
			res = XP_LISP_FVALUE(p1) > XP_LISP_IVALUE(p2);
		}
		else if (XP_LISP_TYPE(p2) == XP_LISP_OBJ_FLOAT) {
			res = XP_LISP_FVALUE(p1) > XP_LISP_FVALUE(p2);
		}
		else {
			lsp->error = XP_LISP_ERR_BAD_VALUE;
			return XP_NULL;
		}
	}
	else if (XP_LISP_TYPE(p1) == XP_LISP_OBJ_SYMBOL) {
		if (XP_LISP_TYPE(p2) == XP_LISP_OBJ_SYMBOL) {
			res = xp_lisp_comp_symbol2 (
				p1, XP_LISP_SYMVALUE(p2), XP_LISP_SYMLEN(p2)) > 0;
		}
		else {
			lsp->error = XP_LISP_ERR_BAD_VALUE;
			return XP_NULL;
		}
	}
	else if (XP_LISP_TYPE(p1) == XP_LISP_OBJ_STRING) {
		if (XP_LISP_TYPE(p2) == XP_LISP_OBJ_STRING) {
			res = xp_lisp_comp_string2 (
				p1, XP_LISP_STRVALUE(p2), XP_LISP_STRLEN(p2)) > 0;
		}
		else {
			lsp->error = XP_LISP_ERR_BAD_VALUE;
			return XP_NULL;
		}
	}
	else {
		lsp->error = XP_LISP_ERR_BAD_VALUE;
		return XP_NULL;
	}

	return (res)? lsp->mem->t: lsp->mem->nil;
}

xp_lisp_obj_t* xp_lisp_prim_lt (xp_lisp_t* lsp, xp_lisp_obj_t* args)
{
	xp_lisp_obj_t* p1, * p2;
	int res;

	XP_LISP_PRIM_CHECK_ARG_COUNT (lsp, args, 2, 2);
	xp_assert (XP_LISP_TYPE(args) == XP_LISP_OBJ_CONS);

	p1 = xp_lisp_eval (lsp, XP_LISP_CAR(args));
	if (p1 == XP_NULL) return XP_NULL;
	// TODO: lock p1....

	p2 = xp_lisp_eval (lsp, XP_LISP_CAR(XP_LISP_CDR(args)));
	if (p2 == XP_NULL) return XP_NULL;

	if (XP_LISP_TYPE(p1) == XP_LISP_OBJ_INT) {
		if (XP_LISP_TYPE(p2) == XP_LISP_OBJ_INT) {
			res = XP_LISP_IVALUE(p1) < XP_LISP_IVALUE(p2);
		}
		else if (XP_LISP_TYPE(p2) == XP_LISP_OBJ_FLOAT) {
			res = XP_LISP_IVALUE(p1) < XP_LISP_FVALUE(p2);
		}
		else {
			lsp->error = XP_LISP_ERR_BAD_VALUE;
			return XP_NULL;
		}
	}
	else if (XP_LISP_TYPE(p1) == XP_LISP_OBJ_FLOAT) {
		if (XP_LISP_TYPE(p2) == XP_LISP_OBJ_INT) {
			res = XP_LISP_FVALUE(p1) < XP_LISP_IVALUE(p2);
		}
		else if (XP_LISP_TYPE(p2) == XP_LISP_OBJ_FLOAT) {
			res = XP_LISP_FVALUE(p1) < XP_LISP_FVALUE(p2);
		}
		else {
			lsp->error = XP_LISP_ERR_BAD_VALUE;
			return XP_NULL;
		}
	}
	else if (XP_LISP_TYPE(p1) == XP_LISP_OBJ_SYMBOL) {
		if (XP_LISP_TYPE(p2) == XP_LISP_OBJ_SYMBOL) {
			res = xp_lisp_comp_symbol2 (
				p1, XP_LISP_SYMVALUE(p2), XP_LISP_SYMLEN(p2)) < 0;
		}
		else {
			lsp->error = XP_LISP_ERR_BAD_VALUE;
			return XP_NULL;
		}
	}
	else if (XP_LISP_TYPE(p1) == XP_LISP_OBJ_STRING) {
		if (XP_LISP_TYPE(p2) == XP_LISP_OBJ_STRING) {
			res = xp_lisp_comp_string2 (
				p1, XP_LISP_STRVALUE(p2), XP_LISP_STRLEN(p2)) < 0;
		}
		else {
			lsp->error = XP_LISP_ERR_BAD_VALUE;
			return XP_NULL;
		}
	}
	else {
		lsp->error = XP_LISP_ERR_BAD_VALUE;
		return XP_NULL;
	}

	return (res)? lsp->mem->t: lsp->mem->nil;
}
