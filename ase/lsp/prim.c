/*
 * $Id: prim.c,v 1.22 2007-02-11 07:36:55 bacon Exp $
 *
 * {License}
 */

#include <ase/lsp/lsp_i.h>

static int __add_prim (ase_lsp_mem_t* mem, 
	const ase_char_t* name, ase_size_t len, 
	ase_lsp_prim_t pimpl, ase_size_t min_args, ase_size_t max_args);

int ase_lsp_addprim (
	ase_lsp_t* lsp, const ase_char_t* name, ase_size_t name_len,
	ase_lsp_prim_t prim, ase_size_t min_args, ase_size_t max_args)
{
	return __add_prim (lsp->mem, name, name_len, prim, min_args, max_args);
}

int ase_lsp_removeprim (ase_lsp_t* lsp, const ase_char_t* name)
{
	// TODO:
	return -1;
}

static int __add_prim (ase_lsp_mem_t* mem, 
	const ase_char_t* name, ase_size_t name_len, 
	ase_lsp_prim_t pimpl, ase_size_t min_args, ase_size_t max_args)
{
	ase_lsp_obj_t* n, * p;
	
	n = ase_lsp_makesym (mem, name, name_len);
	if (n == ASE_NULL) return -1;

	if (ase_lsp_pushtmp (mem->lsp, n) == ASE_NULL) return -1;

	p = ase_lsp_makeprim (mem, pimpl, min_args, max_args);
	if (p == ASE_NULL) 
	{
		ase_lsp_poptmp (mem->lsp);
		return -1;
	}

	if (ase_lsp_pushtmp (mem->lsp, p) == ASE_NULL)
	{
		ase_lsp_poptmp (mem->lsp);
		return -1;
	}

	if (ase_lsp_setfunc(mem, n, p) == ASE_NULL) 
	{
		ase_lsp_poptmp (mem->lsp);
		ase_lsp_poptmp (mem->lsp);
		return -1;
	}

	ase_lsp_poptmp (mem->lsp);
	ase_lsp_poptmp (mem->lsp);
	return 0;
}

ase_lsp_obj_t* ase_lsp_prim_exit (ase_lsp_t* lsp, ase_lsp_obj_t* args)
{
	lsp->errnum = ASE_LSP_EEXIT;
	return ASE_NULL;
}

ase_lsp_obj_t* ase_lsp_prim_eval (ase_lsp_t* lsp, ase_lsp_obj_t* args)
{
	ase_lsp_obj_t* tmp1, * tmp2;

	ASE_LSP_ASSERT (lsp, ASE_LSP_TYPE(args) == ASE_LSP_OBJ_CONS);

	tmp1 = ase_lsp_eval (lsp, ASE_LSP_CAR(args));
	if (tmp1 == ASE_NULL) return ASE_NULL;

	if (ase_lsp_pushtmp (lsp, tmp1) == ASE_NULL) return ASE_NULL;

	tmp2 = ase_lsp_eval (lsp, tmp1);
	if (tmp2 == ASE_NULL) 
	{
		ase_lsp_poptmp (lsp);
		return ASE_NULL;
	}

	ase_lsp_poptmp (lsp);
	return tmp2;
}

ase_lsp_obj_t* ase_lsp_prim_gc (ase_lsp_t* lsp, ase_lsp_obj_t* args)
{
	ase_lsp_gc (lsp->mem);
	return lsp->mem->nil;
}

ase_lsp_obj_t* ase_lsp_prim_cond (ase_lsp_t* lsp, ase_lsp_obj_t* args)
{
	/*
	 * (cond 
	 *     (condition1 result1)
	 *     (consition2 result2)
	 *     ...
	 *     (t resultN))
	 */

	ase_lsp_obj_t* tmp, * ret;

	while (ASE_LSP_TYPE(args) == ASE_LSP_OBJ_CONS) 
	{
		if (ASE_LSP_TYPE(ASE_LSP_CAR(args)) != ASE_LSP_OBJ_CONS) 
		{
			ase_lsp_seterror (lsp, ASE_LSP_EARGBAD, ASE_NULL, 0);
			return ASE_NULL;
		}

		tmp = ase_lsp_eval (lsp, ASE_LSP_CAR(ASE_LSP_CAR(args)));
		if (tmp == ASE_NULL) return ASE_NULL;

		if (ase_lsp_pushtmp (lsp, tmp) == ASE_NULL) return ASE_NULL;

		if (tmp != lsp->mem->nil) 
		{
			int f = 0;

			tmp = ASE_LSP_CDR(ASE_LSP_CAR(args));
			ret = lsp->mem->nil;

			while (ASE_LSP_TYPE(tmp) == ASE_LSP_OBJ_CONS) 
			{
				ret = ase_lsp_eval (lsp, ASE_LSP_CAR(tmp));
				if (ret == ASE_NULL) 
				{
					if (!f) ase_lsp_poptmp (lsp); /* ret */
					ase_lsp_poptmp (lsp); /* tmp */
					return ASE_NULL;
				}

				if (!f) ase_lsp_poptmp (lsp); /* ret */
				if (ase_lsp_pushtmp (lsp, ret) == ASE_NULL) 
				{
					ase_lsp_poptmp (lsp); /* tmp */
					return ASE_NULL;
				}

				f = 1;
				tmp = ASE_LSP_CDR(tmp);
			}
			if (tmp != lsp->mem->nil) 
			{
				if (!f) ase_lsp_poptmp (lsp); /* ret */
				ase_lsp_poptmp (lsp); /* tmp */

				ase_lsp_seterror (lsp, ASE_LSP_EARGBAD, ASE_NULL, 0);
				return ASE_NULL;
			}

			if (!f) ase_lsp_poptmp (lsp); /* ret */
			ase_lsp_poptmp (lsp); /* tmp */
			return ret;
		}

		ase_lsp_poptmp (lsp); /* tmp */
		args = ASE_LSP_CDR(args);
	}

	return lsp->mem->nil;
}

ase_lsp_obj_t* ase_lsp_prim_if (ase_lsp_t* lsp, ase_lsp_obj_t* args)
{
	ase_lsp_obj_t* tmp;

	ASE_LSP_ASSERT (lsp, ASE_LSP_TYPE(args) == ASE_LSP_OBJ_CONS);
	
	tmp = ase_lsp_eval (lsp, ASE_LSP_CAR(args));
	if (tmp == ASE_NULL) return ASE_NULL;

	if (ase_lsp_pushtmp (lsp, tmp) == ASE_NULL) return ASE_NULL;

	if (tmp != lsp->mem->nil) 
	{
		tmp = ase_lsp_eval (lsp, ASE_LSP_CAR(ASE_LSP_CDR(args)));
		if (tmp == ASE_NULL) 
		{
			ase_lsp_poptmp (lsp);  /* tmp */
			return ASE_NULL;
		}

		ase_lsp_poptmp (lsp);  /* tmp */
		return tmp;
	}	
	else 
	{
		ase_lsp_obj_t* res = lsp->mem->nil;
		int f = 0;

		tmp = ASE_LSP_CDR(ASE_LSP_CDR(args));

		while (ASE_LSP_TYPE(tmp) == ASE_LSP_OBJ_CONS) 
		{
			res = ase_lsp_eval (lsp, ASE_LSP_CAR(tmp));
			if (res == ASE_NULL) 
			{
				if (!f) ase_lsp_poptmp (lsp); /* res */
				ase_lsp_poptmp (lsp); /* tmp */
				return ASE_NULL;
			}

			if (!f) ase_lsp_poptmp (lsp); /* res */
			if (ase_lsp_pushtmp (lsp, res) == ASE_NULL)
			{
				ase_lsp_poptmp (lsp); /* tmp */
				return ASE_NULL;
			}

			f = 1;
			tmp = ASE_LSP_CDR(tmp);
		}

		if (tmp != lsp->mem->nil) 
		{
			if (!f) ase_lsp_poptmp (lsp); /* ret */
			ase_lsp_poptmp (lsp); /* tmp */

			ase_lsp_seterror (lsp, ASE_LSP_EARGBAD, ASE_NULL, 0);
			return ASE_NULL;
		}

		if (!f) ase_lsp_poptmp (lsp); /* ret */
		ase_lsp_poptmp (lsp); /* tmp */
		return res;
	}
}

ase_lsp_obj_t* ase_lsp_prim_while (ase_lsp_t* lsp, ase_lsp_obj_t* args)
{
	/*
	 * (setq a 1)
	 * (while (< a 100) (setq a (+ a 1)))
	 */

	ase_lsp_obj_t* tmp;

	ASE_LSP_ASSERT (lsp, ASE_LSP_TYPE(args) == ASE_LSP_OBJ_CONS);

	while (1)
	{
		tmp = ase_lsp_eval (lsp, ASE_LSP_CAR(args));
		if (tmp == ASE_NULL) return ASE_NULL;
		if (tmp == lsp->mem->nil) break;

		if (ase_lsp_pushtmp (lsp, tmp) == ASE_NULL) return ASE_NULL;

		tmp = ASE_LSP_CDR(args);
		while (ASE_LSP_TYPE(tmp) == ASE_LSP_OBJ_CONS) 
		{
			if (ase_lsp_eval(lsp, ASE_LSP_CAR(tmp)) == ASE_NULL)
			{
				ase_lsp_poptmp (lsp); /* tmp */
				return ASE_NULL;
			}

			tmp = ASE_LSP_CDR(tmp);
		}

		if (tmp != lsp->mem->nil) 
		{
			ase_lsp_poptmp (lsp); /* tmp */

			ase_lsp_seterror (lsp, ASE_LSP_EARGBAD, ASE_NULL, 0);
			return ASE_NULL;
		}

		ase_lsp_poptmp (lsp); /* tmp */
	}

	return lsp->mem->nil;
}

ase_lsp_obj_t* ase_lsp_prim_car (ase_lsp_t* lsp, ase_lsp_obj_t* args)
{
	/*
	 * (car '(10 20 30))
	 */

	ase_lsp_obj_t* tmp;

	ASE_LSP_ASSERT (lsp, ASE_LSP_TYPE(args) == ASE_LSP_OBJ_CONS);

	tmp = ase_lsp_eval (lsp, ASE_LSP_CAR(args));
	if (tmp == ASE_NULL) return ASE_NULL;
	if (tmp == lsp->mem->nil) return lsp->mem->nil;

	if (ASE_LSP_TYPE(tmp) != ASE_LSP_OBJ_CONS) 
	{
		ase_lsp_seterror (lsp, ASE_LSP_EARGBAD, ASE_NULL, 0);
		return ASE_NULL;
	}

	return ASE_LSP_CAR(tmp);
}

ase_lsp_obj_t* ase_lsp_prim_cdr (ase_lsp_t* lsp, ase_lsp_obj_t* args)
{
	/*
	 * (cdr '(10 20 30))
	 */

	ase_lsp_obj_t* tmp;

	ASE_LSP_ASSERT (lsp, ASE_LSP_TYPE(args) == ASE_LSP_OBJ_CONS);

	tmp = ase_lsp_eval (lsp, ASE_LSP_CAR(args));
	if (tmp == ASE_NULL) return ASE_NULL;
	if (tmp == lsp->mem->nil) return lsp->mem->nil;

	if (ASE_LSP_TYPE(tmp) != ASE_LSP_OBJ_CONS) 
	{
		ase_lsp_seterror (lsp, ASE_LSP_EARGBAD, ASE_NULL, 0);
		return ASE_NULL;
	}

	return ASE_LSP_CDR(tmp);
}

ase_lsp_obj_t* ase_lsp_prim_cons (ase_lsp_t* lsp, ase_lsp_obj_t* args)
{
	/*
	 * (cons 10 20)
	 * (cons '(10 20) 30)
	 */

	ase_lsp_obj_t* car, * cdr, * cons;

	ASE_LSP_ASSERT (lsp, ASE_LSP_TYPE(args) == ASE_LSP_OBJ_CONS);

	car = ase_lsp_eval (lsp, ASE_LSP_CAR(args));
	if (car == ASE_NULL) return ASE_NULL;

	if (ase_lsp_pushtmp (lsp, car) == ASE_NULL) return ASE_NULL;

	cdr = ase_lsp_eval (lsp, ASE_LSP_CAR(ASE_LSP_CDR(args)));
	if (cdr == ASE_NULL) 
	{
		ase_lsp_poptmp (lsp); /* car */
		return ASE_NULL;
	}

	if (ase_lsp_pushtmp (lsp, cdr) == ASE_NULL) 
	{
		ase_lsp_poptmp (lsp); /* car */
		return ASE_NULL;
	}

	cons = ase_lsp_makecons (lsp->mem, car, cdr);
	if (cons == ASE_NULL) 
	{
		ase_lsp_poptmp (lsp); /* cdr */
		ase_lsp_poptmp (lsp); /* car */
		return ASE_NULL;
	}

	ase_lsp_poptmp (lsp); /* cdr */
	ase_lsp_poptmp (lsp); /* car */
	return cons;
}

ase_lsp_obj_t* ase_lsp_prim_set (ase_lsp_t* lsp, ase_lsp_obj_t* args)
{
	/*
	 * (set 'flowers 'rose)
	 * (set flowers 20)
	 * rose
	 */

	ase_lsp_obj_t* p1, * p2;

	ASE_LSP_ASSERT (lsp, ASE_LSP_TYPE(args) == ASE_LSP_OBJ_CONS);

	p1 = ase_lsp_eval (lsp, ASE_LSP_CAR(args));
	if (p1 == ASE_NULL) return ASE_NULL;

	if (ase_lsp_pushtmp (lsp, p1) == ASE_NULL) return ASE_NULL;

	if (ASE_LSP_TYPE(p1) != ASE_LSP_OBJ_SYM) 
	{
		ase_lsp_poptmp (lsp); /* p1 */

		ase_lsp_seterror (lsp, ASE_LSP_EARGBAD, ASE_NULL, 0);
		return ASE_NULL;
	}

	p2 = ase_lsp_eval (lsp, ASE_LSP_CAR(ASE_LSP_CDR(args)));
	if (p2 == ASE_NULL) 
	{
		ase_lsp_poptmp (lsp); /* p1 */
		return ASE_NULL;
	}

	if (ase_lsp_pushtmp (lsp, p2) == ASE_NULL) 
	{
		ase_lsp_poptmp (lsp); /* p1 */
		return ASE_NULL;
	}

	if (ase_lsp_setvalue (lsp->mem, p1, p2) == ASE_NULL) 
	{
		ase_lsp_poptmp (lsp); /* p2 */
		ase_lsp_poptmp (lsp); /* p1 */
		return ASE_NULL;
	}

	ase_lsp_poptmp (lsp); /* p2 */
	ase_lsp_poptmp (lsp); /* p1 */
	return p2;
}

ase_lsp_obj_t* ase_lsp_prim_setq (ase_lsp_t* lsp, ase_lsp_obj_t* args)
{
	/*
	 * (setq x 10)
	 * (setq x "string")
	 */

	ase_lsp_obj_t* p = args, * p1, * p2 = lsp->mem->nil;

	while (p != lsp->mem->nil) 
	{
		ASE_LSP_ASSERT (lsp, ASE_LSP_TYPE(p) == ASE_LSP_OBJ_CONS);

		p1 = ASE_LSP_CAR(p);
		if (ASE_LSP_TYPE(p1) != ASE_LSP_OBJ_SYM) 
		{
			ase_lsp_seterror (lsp, ASE_LSP_EARGBAD, ASE_NULL, 0);
			return ASE_NULL;
		}

		if (ASE_LSP_TYPE(ASE_LSP_CDR(p)) != ASE_LSP_OBJ_CONS) 
		{
			lsp->errnum = ASE_LSP_EARGFEW;
			return ASE_NULL;
		}

		p2 = ase_lsp_eval (lsp, ASE_LSP_CAR(ASE_LSP_CDR(p)));
		if (p2 == ASE_NULL) return ASE_NULL;

		if (ase_lsp_pushtmp (lsp, p2) == ASE_NULL) return ASE_NULL;

		if (ase_lsp_setvalue (lsp->mem, p1, p2) == ASE_NULL) 
		{
			ase_lsp_poptmp (lsp);
			return ASE_NULL;
		}

		ase_lsp_poptmp (lsp);
		p = ASE_LSP_CDR(ASE_LSP_CDR(p));
	}

	return p2;
}

ase_lsp_obj_t* ase_lsp_prim_quote (ase_lsp_t* lsp, ase_lsp_obj_t* args)
{
	/*
	 * (quote (10 20 30 50))
	 */

	ASE_LSP_ASSERT (lsp, ASE_LSP_TYPE(args) == ASE_LSP_OBJ_CONS);
	return ASE_LSP_CAR(args);
}

ase_lsp_obj_t* ase_lsp_prim_defun (ase_lsp_t* lsp, ase_lsp_obj_t* args)
{
	/*
	 * (defun x (a b)  (+ a b 100))
	 * (x 40 50)
	 *
	 * (setq x (lambda (x y) (setq temp 10) (+ x y temp)))
	 * (x 40 50)
	 * temp 
	 */

	ase_lsp_obj_t* name, * fun;

	name = ASE_LSP_CAR(args);
	if (ASE_LSP_TYPE(name) != ASE_LSP_OBJ_SYM) 
	{
		ase_lsp_seterror (lsp, ASE_LSP_EARGBAD, ASE_NULL, 0);
		return ASE_NULL;
	}

	fun = ase_lsp_makefunc (lsp->mem, 
		ASE_LSP_CAR(ASE_LSP_CDR(args)), ASE_LSP_CDR(ASE_LSP_CDR(args)));
	if (fun == ASE_NULL) return ASE_NULL;

	if (ase_lsp_pushtmp (lsp, fun) == ASE_NULL) return ASE_NULL;

	if (ase_lsp_setfunc (lsp->mem, ASE_LSP_CAR(args), fun) == ASE_NULL) 
	{
		ase_lsp_poptmp (lsp);
		return ASE_NULL;
	}

	ase_lsp_poptmp (lsp);
	return fun;
}

ase_lsp_obj_t* ase_lsp_prim_demac (ase_lsp_t* lsp, ase_lsp_obj_t* args)
{
	/*
	 * (demac x (abc)  x y z)
	 * (setq x (macro (abc) x y z))
	 */

	ase_lsp_obj_t* name, * mac;

	name = ASE_LSP_CAR(args);
	if (ASE_LSP_TYPE(name) != ASE_LSP_OBJ_SYM) 
	{
		ase_lsp_seterror (lsp, ASE_LSP_EARGBAD, ASE_NULL, 0);
		return ASE_NULL;
	}

	mac = ase_lsp_makemacro (lsp->mem, 
		ASE_LSP_CAR(ASE_LSP_CDR(args)), ASE_LSP_CDR(ASE_LSP_CDR(args)));
	if (mac == ASE_NULL) return ASE_NULL;

	if (ase_lsp_pushtmp (lsp, mac) == ASE_NULL) return ASE_NULL;

	if (ase_lsp_setfunc (lsp->mem, ASE_LSP_CAR(args), mac) == ASE_NULL) 
	{
		ase_lsp_poptmp (lsp);
		return ASE_NULL;
	}

	ase_lsp_poptmp (lsp);
	return mac;
}

ase_lsp_obj_t* ase_lsp_prim_or (ase_lsp_t* lsp, ase_lsp_obj_t* args)
{
	/*
	 * (or 10 20 30 40)
	 * (or (= n 20) (= n 30))
	 */
	ase_lsp_obj_t* tmp;

/* TODO: this is wrong. redo the work */
	while (ASE_LSP_TYPE(args) == ASE_LSP_OBJ_CONS) 
	{
		tmp = ase_lsp_eval (lsp, ASE_LSP_CAR(args));
		if (tmp == ASE_NULL) return ASE_NULL;

		if (ASE_LSP_TYPE(args) == ASE_LSP_OBJ_INT)
		if (tmp != lsp->mem->nil) return lsp->mem->t;
		args = ASE_LSP_CDR(args);
	}

	return lsp->mem->nil;
}

