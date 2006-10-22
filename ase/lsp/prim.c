/*
 * $Id: prim.c,v 1.9 2006-10-22 13:10:46 bacon Exp $
 */

#include <sse/lsp/lsp.h>
#include <sse/lsp/mem.h>
#include <sse/lsp/prim.h>

#include <sse/bas/string.h>
#include <sse/bas/assert.h>

static int __add_prim (sse_lsp_mem_t* mem, 
	const sse_char_t* name, sse_size_t len, sse_lsp_prim_t prim);

int sse_lsp_add_prim (
	sse_lsp_t* lsp, const sse_char_t* name, sse_lsp_prim_t prim)
{
	return __add_prim (lsp->mem, name, sse_strlen(name), prim);
}

int sse_lsp_remove_prim (sse_lsp_t* lsp, const sse_char_t* name)
{
	// TODO:
	return -1;
}

static int __add_prim (sse_lsp_mem_t* mem, 
	const sse_char_t* name, sse_size_t len, sse_lsp_prim_t prim)
{
	sse_lsp_obj_t* n, * p;
	
	n = sse_lsp_make_symbolx (mem, name, len);
	if (n == SSE_NULL) return -1;

	sse_lsp_lock (n);

	p = sse_lsp_make_prim (mem, prim);
	if (p == SSE_NULL) return -1;

	sse_lsp_unlock (n);

	if (sse_lsp_set_func(mem, n, p) == SSE_NULL) return -1;

	return 0;
}

sse_lsp_obj_t* sse_lsp_prim_abort (sse_lsp_t* lsp, sse_lsp_obj_t* args)
{
	SSE_LSP_PRIM_CHECK_ARG_COUNT (lsp, args, 0, 0);
	lsp->errnum = SSE_LSP_ERR_ABORT;
	return SSE_NULL;
}

sse_lsp_obj_t* sse_lsp_prim_eval (sse_lsp_t* lsp, sse_lsp_obj_t* args)
{
	sse_lsp_obj_t* tmp;

	SSE_LSP_PRIM_CHECK_ARG_COUNT (lsp, args, 1, 1);
	sse_assert (SSE_LSP_TYPE(args) == SSE_LSP_OBJ_CONS);

	tmp = sse_lsp_eval (lsp, SSE_LSP_CAR(args));
	if (tmp == SSE_NULL) return SSE_NULL;

	tmp = sse_lsp_eval (lsp, tmp);
	if (tmp == SSE_NULL) return SSE_NULL;

	return tmp;
}

sse_lsp_obj_t* sse_lsp_prim_gc (sse_lsp_t* lsp, sse_lsp_obj_t* args)
{
	SSE_LSP_PRIM_CHECK_ARG_COUNT (lsp, args, 0, 0);
	sse_lsp_garbage_collect (lsp->mem);
	return lsp->mem->nil;
}

sse_lsp_obj_t* sse_lsp_prim_cond (sse_lsp_t* lsp, sse_lsp_obj_t* args)
{
	/*
	 * (cond 
	 *     (condition1 result1)
	 *     (consition2 result2)
	 *     ...
	 *     (t resultN))
	 */

	sse_lsp_obj_t* tmp, * ret;

	SSE_LSP_PRIM_CHECK_ARG_COUNT (lsp, args, 0, SSE_LSP_PRIM_MAX_ARG_COUNT);

	while (SSE_LSP_TYPE(args) == SSE_LSP_OBJ_CONS) {
		if (SSE_LSP_TYPE(SSE_LSP_CAR(args)) != SSE_LSP_OBJ_CONS) {
			lsp->errnum = SSE_LSP_ERR_BAD_ARG;
			return SSE_NULL;
		}

		tmp = sse_lsp_eval (lsp, SSE_LSP_CAR(SSE_LSP_CAR(args)));
		if (tmp == SSE_NULL) return SSE_NULL;

		if (tmp != lsp->mem->nil) {
			tmp = SSE_LSP_CDR(SSE_LSP_CAR(args));
			ret = lsp->mem->nil;
			while (SSE_LSP_TYPE(tmp) == SSE_LSP_OBJ_CONS) {
				ret = sse_lsp_eval (lsp, SSE_LSP_CAR(tmp));
				if (ret == SSE_NULL) return SSE_NULL;
				tmp = SSE_LSP_CDR(tmp);
			}
			if (tmp != lsp->mem->nil) {
				lsp->errnum = SSE_LSP_ERR_BAD_ARG;
				return SSE_NULL;
			}
			return ret;
		}

		args = SSE_LSP_CDR(args);
	}

	return lsp->mem->nil;
}

sse_lsp_obj_t* sse_lsp_prim_if (sse_lsp_t* lsp, sse_lsp_obj_t* args)
{
	sse_lsp_obj_t* tmp;

	SSE_LSP_PRIM_CHECK_ARG_COUNT (lsp, args, 2, SSE_LSP_PRIM_MAX_ARG_COUNT);
	sse_assert (SSE_LSP_TYPE(args) == SSE_LSP_OBJ_CONS);
	
	tmp = sse_lsp_eval (lsp, SSE_LSP_CAR(args));
	if (tmp == SSE_NULL) return SSE_NULL;

	if (tmp != lsp->mem->nil) {
		tmp = sse_lsp_eval (lsp, SSE_LSP_CAR(SSE_LSP_CDR(args)));
		if (tmp == SSE_NULL) return SSE_NULL;
		return tmp;
	}	
	else {
		sse_lsp_obj_t* res = lsp->mem->nil;

		tmp = SSE_LSP_CDR(SSE_LSP_CDR(args));

		while (SSE_LSP_TYPE(tmp) == SSE_LSP_OBJ_CONS) {
			res = sse_lsp_eval (lsp, SSE_LSP_CAR(tmp));
			if (res == SSE_NULL) return SSE_NULL;
			tmp = SSE_LSP_CDR(tmp);
		}
		if (tmp != lsp->mem->nil) {
			lsp->errnum = SSE_LSP_ERR_BAD_ARG;
			return SSE_NULL;
		}

		return res;
	}
}

sse_lsp_obj_t* sse_lsp_prim_while (sse_lsp_t* lsp, sse_lsp_obj_t* args)
{
	/*
	 * (setq a 1)
	 * (while (< a 100) (setq a (+ a 1)))
	 */

	sse_lsp_obj_t* tmp;

	SSE_LSP_PRIM_CHECK_ARG_COUNT (lsp, args, 1, SSE_LSP_PRIM_MAX_ARG_COUNT);
	sse_assert (SSE_LSP_TYPE(args) == SSE_LSP_OBJ_CONS);

	for (;;) {
		tmp = sse_lsp_eval (lsp, SSE_LSP_CAR(args));
		if (tmp == SSE_NULL) return SSE_NULL;
		if (tmp == lsp->mem->nil) break;

		tmp = SSE_LSP_CDR(args);
		while (SSE_LSP_TYPE(tmp) == SSE_LSP_OBJ_CONS) {
			if (sse_lsp_eval(lsp, SSE_LSP_CAR(tmp)) == SSE_NULL) 
				return SSE_NULL;
			tmp = SSE_LSP_CDR(tmp);
		}

		if (tmp != lsp->mem->nil) {
			lsp->errnum = SSE_LSP_ERR_BAD_ARG;
			return SSE_NULL;
		}
	}

	return lsp->mem->nil;
}

sse_lsp_obj_t* sse_lsp_prim_car (sse_lsp_t* lsp, sse_lsp_obj_t* args)
{
	/*
	 * (car '(10 20 30))
	 */

	sse_lsp_obj_t* tmp;

	SSE_LSP_PRIM_CHECK_ARG_COUNT (lsp, args, 1, 1);
	sse_assert (SSE_LSP_TYPE(args) == SSE_LSP_OBJ_CONS);

	tmp = sse_lsp_eval (lsp, SSE_LSP_CAR(args));
	if (tmp == SSE_NULL) return SSE_NULL;
	if (tmp == lsp->mem->nil) return lsp->mem->nil;

	if (SSE_LSP_TYPE(tmp) != SSE_LSP_OBJ_CONS) {
		lsp->errnum = SSE_LSP_ERR_BAD_ARG;
		return SSE_NULL;
	}

	return SSE_LSP_CAR(tmp);
}

sse_lsp_obj_t* sse_lsp_prim_cdr (sse_lsp_t* lsp, sse_lsp_obj_t* args)
{
	/*
	 * (cdr '(10 20 30))
	 */

	sse_lsp_obj_t* tmp;

	SSE_LSP_PRIM_CHECK_ARG_COUNT (lsp, args, 1, 1);
	sse_assert (SSE_LSP_TYPE(args) == SSE_LSP_OBJ_CONS);

	tmp = sse_lsp_eval (lsp, SSE_LSP_CAR(args));
	if (tmp == SSE_NULL) return SSE_NULL;
	if (tmp == lsp->mem->nil) return lsp->mem->nil;

	if (SSE_LSP_TYPE(tmp) != SSE_LSP_OBJ_CONS) {
		lsp->errnum = SSE_LSP_ERR_BAD_ARG;
		return SSE_NULL;
	}

	return SSE_LSP_CDR(tmp);
}

sse_lsp_obj_t* sse_lsp_prim_cons (sse_lsp_t* lsp, sse_lsp_obj_t* args)
{
	/*
	 * (cons 10 20)
	 * (cons '(10 20) 30)
	 */

	sse_lsp_obj_t* car, * cdr, * cons;

	SSE_LSP_PRIM_CHECK_ARG_COUNT (lsp, args, 2, 2);
	sse_assert (SSE_LSP_TYPE(args) == SSE_LSP_OBJ_CONS);

	car = sse_lsp_eval (lsp, SSE_LSP_CAR(args));
	if (car == SSE_NULL) return SSE_NULL;

	cdr = sse_lsp_eval (lsp, SSE_LSP_CAR(SSE_LSP_CDR(args)));
	if (cdr == SSE_NULL) return SSE_NULL;

	cons = sse_lsp_make_cons (lsp->mem, car, cdr);
	if (cons == SSE_NULL) {
		lsp->errnum = SSE_LSP_ERR_MEMORY;
		return SSE_NULL;
	}

	return cons;
}

sse_lsp_obj_t* sse_lsp_prim_set (sse_lsp_t* lsp, sse_lsp_obj_t* args)
{
	/*
	 * (set 'flowers 'rose)
	 * (set flowers 20)
	 * (rose)
	 */

	sse_lsp_obj_t* p1, * p2;

	SSE_LSP_PRIM_CHECK_ARG_COUNT (lsp, args, 2, 2);
	sse_assert (SSE_LSP_TYPE(args) == SSE_LSP_OBJ_CONS);

	p1 = sse_lsp_eval (lsp, SSE_LSP_CAR(args));
	if (p1 == SSE_NULL) return SSE_NULL;

	if (SSE_LSP_TYPE(p1) != SSE_LSP_OBJ_SYMBOL) {
		lsp->errnum = SSE_LSP_ERR_BAD_ARG;
		return SSE_NULL;
	}

	p2 = sse_lsp_eval (lsp, SSE_LSP_CAR(SSE_LSP_CDR(args)));
	if (p2 == SSE_NULL) return SSE_NULL;

	if (sse_lsp_set_value (lsp->mem, p1, p2) == SSE_NULL) {
		lsp->errnum = SSE_LSP_ERR_MEMORY;
		return SSE_NULL;
	}

	return p2;
}

sse_lsp_obj_t* sse_lsp_prim_setq (sse_lsp_t* lsp, sse_lsp_obj_t* args)
{
	/*
	 * (setq x 10)
	 * (setq x "stirng")
	 */

	sse_lsp_obj_t* p = args, * p1, * p2 = lsp->mem->nil;

	while (p != lsp->mem->nil) {
		sse_assert (SSE_LSP_TYPE(p) == SSE_LSP_OBJ_CONS);

		p1 = SSE_LSP_CAR(p);
		if (SSE_LSP_TYPE(p1) != SSE_LSP_OBJ_SYMBOL) {
			lsp->errnum = SSE_LSP_ERR_BAD_ARG;
			return SSE_NULL;
		}

		if (SSE_LSP_TYPE(SSE_LSP_CDR(p)) != SSE_LSP_OBJ_CONS) {
			lsp->errnum = SSE_LSP_ERR_TOO_FEW_ARGS;
			return SSE_NULL;
		}

		p2 = sse_lsp_eval (lsp, SSE_LSP_CAR(SSE_LSP_CDR(p)));
		if (p2 == SSE_NULL) return SSE_NULL;

		if (sse_lsp_set_value (lsp->mem, p1, p2) == SSE_NULL) {
			lsp->errnum = SSE_LSP_ERR_MEMORY;
			return SSE_NULL;
		}

		p = SSE_LSP_CDR(SSE_LSP_CDR(p));
	}

	return p2;
}

sse_lsp_obj_t* sse_lsp_prim_quote (sse_lsp_t* lsp, sse_lsp_obj_t* args)
{
	/*
	 * (quote (10 20 30 50))
	 */

	SSE_LSP_PRIM_CHECK_ARG_COUNT (lsp, args, 1, 1);
	sse_assert (SSE_LSP_TYPE(args) == SSE_LSP_OBJ_CONS);
	return SSE_LSP_CAR(args);
}

sse_lsp_obj_t* sse_lsp_prim_defun (sse_lsp_t* lsp, sse_lsp_obj_t* args)
{
	/*
	 * (defun x (a b)  (+ a b 100))
	 * (x 40 50)
	 *
	 * (setq x (lambda (x y) (setq temp 10) (+ x y temp)))
	 * (x 40 50)
	 * temp 
	 */

	sse_lsp_obj_t* name, * fun;

	SSE_LSP_PRIM_CHECK_ARG_COUNT (lsp, args, 3, SSE_LSP_PRIM_MAX_ARG_COUNT);

	name = SSE_LSP_CAR(args);
	if (SSE_LSP_TYPE(name) != SSE_LSP_OBJ_SYMBOL) {
		lsp->errnum = SSE_LSP_ERR_BAD_ARG;
		return SSE_NULL;
	}

	fun = sse_lsp_make_func (lsp->mem, 
		SSE_LSP_CAR(SSE_LSP_CDR(args)), SSE_LSP_CDR(SSE_LSP_CDR(args)));
	if (fun == SSE_NULL) return SSE_NULL;

	if (sse_lsp_set_func (lsp->mem, SSE_LSP_CAR(args), fun) == SSE_NULL) {
		lsp->errnum = SSE_LSP_ERR_MEMORY;
		return SSE_NULL;
	}
	return fun;
}

sse_lsp_obj_t* sse_lsp_prim_demac (sse_lsp_t* lsp, sse_lsp_obj_t* args)
{
	/*
	 * (demac x (abc)  x y z)
	 *(setq x (macro (abc) x y z))
	 */

	sse_lsp_obj_t* name, * mac;

	SSE_LSP_PRIM_CHECK_ARG_COUNT (lsp, args, 3, SSE_LSP_PRIM_MAX_ARG_COUNT);

	name = SSE_LSP_CAR(args);
	if (SSE_LSP_TYPE(name) != SSE_LSP_OBJ_SYMBOL) {
		lsp->errnum = SSE_LSP_ERR_BAD_ARG;
		return SSE_NULL;
	}

	mac = sse_lsp_make_macro (lsp->mem, 
		SSE_LSP_CAR(SSE_LSP_CDR(args)), SSE_LSP_CDR(SSE_LSP_CDR(args)));
	if (mac == SSE_NULL) return SSE_NULL;

	if (sse_lsp_set_func (lsp->mem, SSE_LSP_CAR(args), mac) == SSE_NULL) {
		lsp->errnum = SSE_LSP_ERR_MEMORY;
		return SSE_NULL;
	}
	return mac;
}
