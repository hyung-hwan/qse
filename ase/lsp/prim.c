/*
 * $Id: prim.c,v 1.6 2005-09-20 11:19:15 bacon Exp $
 */

#include <xp/lsp/lsp.h>
#include <xp/lsp/mem.h>
#include <xp/lsp/prim.h>

#include <xp/bas/string.h>
#include <xp/bas/assert.h>

static int __add_prim (xp_lsp_mem_t* mem, 
	const xp_char_t* name, xp_size_t len, xp_lsp_prim_t prim);

int xp_lsp_add_prim (
	xp_lsp_t* lsp, const xp_char_t* name, xp_lsp_prim_t prim)
{
	return __add_prim (lsp->mem, name, xp_strlen(name), prim);
}

int xp_lsp_remove_prim (xp_lsp_t* lsp, const xp_char_t* name)
{
	// TODO:
	return -1;
}

static int __add_prim (xp_lsp_mem_t* mem, 
	const xp_char_t* name, xp_size_t len, xp_lsp_prim_t prim)
{
	xp_lsp_obj_t* n, * p;
	
	n = xp_lsp_make_symbolx (mem, name, len);
	if (n == XP_NULL) return -1;

	xp_lsp_lock (n);

	p = xp_lsp_make_prim (mem, prim);
	if (p == XP_NULL) return -1;

	xp_lsp_unlock (n);

	if (xp_lsp_set_func(mem, n, p) == XP_NULL) return -1;

	return 0;
}



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
			if (xp_lsp_eval(lsp, XP_LSP_CAR(tmp)) == XP_NULL) return XP_NULL;
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
	/*
	 * (car '(10 20 30))
	 */

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
	/*
	 * (cdr '(10 20 30))
	 */

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
	/*
	 * (cons 10 20)
	 * (cons '(10 20) 30)
	 */

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
	/*
	 * (set 'flowers 'rose)
	 * (set flowers 20)
	 * (rose)
	 */

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

	if (xp_lsp_set_value (lsp->mem, p1, p2) == XP_NULL) {
		lsp->errnum = XP_LSP_ERR_MEM;
		return XP_NULL;
	}

	return p2;
}

xp_lsp_obj_t* xp_lsp_prim_setq (xp_lsp_t* lsp, xp_lsp_obj_t* args)
{
	/*
	 * (setq x 10)
	 * (setq x "stirng")
	 */

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

		if (xp_lsp_set_value (lsp->mem, p1, p2) == XP_NULL) {
			lsp->errnum = XP_LSP_ERR_MEM;
			return XP_NULL;
		}

		p = XP_LSP_CDR(XP_LSP_CDR(p));
	}

	return p2;
}

xp_lsp_obj_t* xp_lsp_prim_quote (xp_lsp_t* lsp, xp_lsp_obj_t* args)
{
	/*
	 * (quote (10 20 30 50))
	 */

	XP_LSP_PRIM_CHECK_ARG_COUNT (lsp, args, 1, 1);
	xp_assert (XP_LSP_TYPE(args) == XP_LSP_OBJ_CONS);
	return XP_LSP_CAR(args);
}

xp_lsp_obj_t* xp_lsp_prim_defun (xp_lsp_t* lsp, xp_lsp_obj_t* args)
{
	/*
	 * (defun x (a b)  (+ a b 100))
	 * (x 40 50)
	 *
	 * (setq x (lambda (x y) (setq temp 10) (+ x y temp)))
	 * (x 40 50)
	 * temp 
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

	if (xp_lsp_set_func (lsp->mem, XP_LSP_CAR(args), fun) == XP_NULL) {
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

	if (xp_lsp_set_func (lsp->mem, XP_LSP_CAR(args), mac) == XP_NULL) {
		lsp->errnum = XP_LSP_ERR_MEM;
		return XP_NULL;
	}
	return mac;
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
		else if (XP_LSP_TYPE(p2) == XP_LSP_OBJ_REAL) {
			res = XP_LSP_IVALUE(p1) > XP_LSP_RVALUE(p2);
		}
		else {
			lsp->errnum = XP_LSP_ERR_BAD_VALUE;
			return XP_NULL;
		}
	}
	else if (XP_LSP_TYPE(p1) == XP_LSP_OBJ_REAL) {
		if (XP_LSP_TYPE(p2) == XP_LSP_OBJ_INT) {
			res = XP_LSP_RVALUE(p1) > XP_LSP_IVALUE(p2);
		}
		else if (XP_LSP_TYPE(p2) == XP_LSP_OBJ_REAL) {
			res = XP_LSP_RVALUE(p1) > XP_LSP_RVALUE(p2);
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
		else if (XP_LSP_TYPE(p2) == XP_LSP_OBJ_REAL) {
			res = XP_LSP_IVALUE(p1) < XP_LSP_RVALUE(p2);
		}
		else {
			lsp->errnum = XP_LSP_ERR_BAD_VALUE;
			return XP_NULL;
		}
	}
	else if (XP_LSP_TYPE(p1) == XP_LSP_OBJ_REAL) {
		if (XP_LSP_TYPE(p2) == XP_LSP_OBJ_INT) {
			res = XP_LSP_RVALUE(p1) < XP_LSP_IVALUE(p2);
		}
		else if (XP_LSP_TYPE(p2) == XP_LSP_OBJ_REAL) {
			res = XP_LSP_RVALUE(p1) < XP_LSP_RVALUE(p2);
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
