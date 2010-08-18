/*
 * $Id: prim.c 337 2008-08-20 09:17:25Z baconevi $
 *
 * {License}
 */

#include "lsp.h"

static int __add_prim (qse_lsp_mem_t* mem, 
	const qse_char_t* name, qse_size_t len, 
	qse_lsp_prim_t pimpl, qse_size_t min_args, qse_size_t max_args);

int qse_lsp_addprim (
	qse_lsp_t* lsp, const qse_char_t* name, qse_size_t name_len,
	qse_lsp_prim_t prim, qse_size_t min_args, qse_size_t max_args)
{
	return __add_prim (lsp->mem, name, name_len, prim, min_args, max_args);
}

int qse_lsp_removeprim (qse_lsp_t* lsp, const qse_char_t* name)
{
	/* TODO: */
	return -1;
}

static int __add_prim (qse_lsp_mem_t* mem, 
	const qse_char_t* name, qse_size_t name_len, 
	qse_lsp_prim_t pimpl, qse_size_t min_args, qse_size_t max_args)
{
	qse_lsp_obj_t* n, * p;
	
	n = qse_lsp_makesym (mem, name, name_len);
	if (n == QSE_NULL) return -1;

	if (qse_lsp_pushtmp (mem->lsp, n) == QSE_NULL) return -1;

	p = qse_lsp_makeprim (mem, pimpl, min_args, max_args);
	if (p == QSE_NULL) 
	{
		qse_lsp_poptmp (mem->lsp);
		return -1;
	}

	if (qse_lsp_pushtmp (mem->lsp, p) == QSE_NULL)
	{
		qse_lsp_poptmp (mem->lsp);
		return -1;
	}

	if (qse_lsp_setfunc(mem, n, p) == QSE_NULL) 
	{
		qse_lsp_poptmp (mem->lsp);
		qse_lsp_poptmp (mem->lsp);
		return -1;
	}

	qse_lsp_poptmp (mem->lsp);
	qse_lsp_poptmp (mem->lsp);
	return 0;
}

qse_lsp_obj_t* qse_lsp_prim_exit (qse_lsp_t* lsp, qse_lsp_obj_t* args)
{
	lsp->errnum = QSE_LSP_EEXIT;
	return QSE_NULL;
}

qse_lsp_obj_t* qse_lsp_prim_eval (qse_lsp_t* lsp, qse_lsp_obj_t* args)
{
	qse_lsp_obj_t* tmp1, * tmp2;

	QSE_ASSERT (QSE_LSP_TYPE(args) == QSE_LSP_OBJ_CONS);

	tmp1 = qse_lsp_eval (lsp, QSE_LSP_CAR(args));
	if (tmp1 == QSE_NULL) return QSE_NULL;

	if (qse_lsp_pushtmp (lsp, tmp1) == QSE_NULL) return QSE_NULL;

	tmp2 = qse_lsp_eval (lsp, tmp1);
	if (tmp2 == QSE_NULL) 
	{
		qse_lsp_poptmp (lsp);
		return QSE_NULL;
	}

	qse_lsp_poptmp (lsp);
	return tmp2;
}

qse_lsp_obj_t* qse_lsp_prim_gc (qse_lsp_t* lsp, qse_lsp_obj_t* args)
{
	qse_lsp_gc (lsp->mem);
	return lsp->mem->nil;
}

qse_lsp_obj_t* qse_lsp_prim_cond (qse_lsp_t* lsp, qse_lsp_obj_t* args)
{
	/*
	 * (cond 
	 *     (condition1 result1)
	 *     (consition2 result2)
	 *     ...
	 *     (t resultN))
	 */

	qse_lsp_obj_t* tmp, * ret;

	while (QSE_LSP_TYPE(args) == QSE_LSP_OBJ_CONS) 
	{
		if (QSE_LSP_TYPE(QSE_LSP_CAR(args)) != QSE_LSP_OBJ_CONS) 
		{
			qse_lsp_seterror (lsp, QSE_LSP_EARGBAD, QSE_NULL, 0);
			return QSE_NULL;
		}

		tmp = qse_lsp_eval (lsp, QSE_LSP_CAR(QSE_LSP_CAR(args)));
		if (tmp == QSE_NULL) return QSE_NULL;

		if (qse_lsp_pushtmp (lsp, tmp) == QSE_NULL) return QSE_NULL;

		if (tmp != lsp->mem->nil) 
		{
			int f = 0;

			tmp = QSE_LSP_CDR(QSE_LSP_CAR(args));
			ret = lsp->mem->nil;

			while (QSE_LSP_TYPE(tmp) == QSE_LSP_OBJ_CONS) 
			{
				ret = qse_lsp_eval (lsp, QSE_LSP_CAR(tmp));
				if (ret == QSE_NULL) 
				{
					if (!f) qse_lsp_poptmp (lsp); /* ret */
					qse_lsp_poptmp (lsp); /* tmp */
					return QSE_NULL;
				}

				if (!f) qse_lsp_poptmp (lsp); /* ret */
				if (qse_lsp_pushtmp (lsp, ret) == QSE_NULL) 
				{
					qse_lsp_poptmp (lsp); /* tmp */
					return QSE_NULL;
				}

				f = 1;
				tmp = QSE_LSP_CDR(tmp);
			}
			if (tmp != lsp->mem->nil) 
			{
				if (!f) qse_lsp_poptmp (lsp); /* ret */
				qse_lsp_poptmp (lsp); /* tmp */

				qse_lsp_seterror (lsp, QSE_LSP_EARGBAD, QSE_NULL, 0);
				return QSE_NULL;
			}

			if (!f) qse_lsp_poptmp (lsp); /* ret */
			qse_lsp_poptmp (lsp); /* tmp */
			return ret;
		}

		qse_lsp_poptmp (lsp); /* tmp */
		args = QSE_LSP_CDR(args);
	}

	return lsp->mem->nil;
}

qse_lsp_obj_t* qse_lsp_prim_if (qse_lsp_t* lsp, qse_lsp_obj_t* args)
{
	qse_lsp_obj_t* tmp;

	QSE_ASSERT (QSE_LSP_TYPE(args) == QSE_LSP_OBJ_CONS);
	
	tmp = qse_lsp_eval (lsp, QSE_LSP_CAR(args));
	if (tmp == QSE_NULL) return QSE_NULL;

	if (qse_lsp_pushtmp (lsp, tmp) == QSE_NULL) return QSE_NULL;

	if (tmp != lsp->mem->nil) 
	{
		tmp = qse_lsp_eval (lsp, QSE_LSP_CAR(QSE_LSP_CDR(args)));
		if (tmp == QSE_NULL) 
		{
			qse_lsp_poptmp (lsp);  /* tmp */
			return QSE_NULL;
		}

		qse_lsp_poptmp (lsp);  /* tmp */
		return tmp;
	}	
	else 
	{
		qse_lsp_obj_t* res = lsp->mem->nil;
		int f = 0;

		tmp = QSE_LSP_CDR(QSE_LSP_CDR(args));

		while (QSE_LSP_TYPE(tmp) == QSE_LSP_OBJ_CONS) 
		{
			res = qse_lsp_eval (lsp, QSE_LSP_CAR(tmp));
			if (res == QSE_NULL) 
			{
				if (!f) qse_lsp_poptmp (lsp); /* res */
				qse_lsp_poptmp (lsp); /* tmp */
				return QSE_NULL;
			}

			if (!f) qse_lsp_poptmp (lsp); /* res */
			if (qse_lsp_pushtmp (lsp, res) == QSE_NULL)
			{
				qse_lsp_poptmp (lsp); /* tmp */
				return QSE_NULL;
			}

			f = 1;
			tmp = QSE_LSP_CDR(tmp);
		}

		if (tmp != lsp->mem->nil) 
		{
			if (!f) qse_lsp_poptmp (lsp); /* ret */
			qse_lsp_poptmp (lsp); /* tmp */

			qse_lsp_seterror (lsp, QSE_LSP_EARGBAD, QSE_NULL, 0);
			return QSE_NULL;
		}

		if (!f) qse_lsp_poptmp (lsp); /* ret */
		qse_lsp_poptmp (lsp); /* tmp */
		return res;
	}
}

qse_lsp_obj_t* qse_lsp_prim_while (qse_lsp_t* lsp, qse_lsp_obj_t* args)
{
	/*
	 * (setq a 1)
	 * (while (< a 100) (setq a (+ a 1)))
	 */

	qse_lsp_obj_t* tmp;

	QSE_ASSERT (QSE_LSP_TYPE(args) == QSE_LSP_OBJ_CONS);

	while (1)
	{
		tmp = qse_lsp_eval (lsp, QSE_LSP_CAR(args));
		if (tmp == QSE_NULL) return QSE_NULL;
		if (tmp == lsp->mem->nil) break;

		if (qse_lsp_pushtmp (lsp, tmp) == QSE_NULL) return QSE_NULL;

		tmp = QSE_LSP_CDR(args);
		while (QSE_LSP_TYPE(tmp) == QSE_LSP_OBJ_CONS) 
		{
			if (qse_lsp_eval(lsp, QSE_LSP_CAR(tmp)) == QSE_NULL)
			{
				qse_lsp_poptmp (lsp); /* tmp */
				return QSE_NULL;
			}

			tmp = QSE_LSP_CDR(tmp);
		}

		if (tmp != lsp->mem->nil) 
		{
			qse_lsp_poptmp (lsp); /* tmp */

			qse_lsp_seterror (lsp, QSE_LSP_EARGBAD, QSE_NULL, 0);
			return QSE_NULL;
		}

		qse_lsp_poptmp (lsp); /* tmp */
	}

	return lsp->mem->nil;
}

qse_lsp_obj_t* qse_lsp_prim_car (qse_lsp_t* lsp, qse_lsp_obj_t* args)
{
	/*
	 * (car '(10 20 30))
	 */

	qse_lsp_obj_t* tmp;

	QSE_ASSERT (QSE_LSP_TYPE(args) == QSE_LSP_OBJ_CONS);

	tmp = qse_lsp_eval (lsp, QSE_LSP_CAR(args));
	if (tmp == QSE_NULL) return QSE_NULL;
	if (tmp == lsp->mem->nil) return lsp->mem->nil;

	if (QSE_LSP_TYPE(tmp) != QSE_LSP_OBJ_CONS) 
	{
		qse_lsp_seterror (lsp, QSE_LSP_EARGBAD, QSE_NULL, 0);
		return QSE_NULL;
	}

	return QSE_LSP_CAR(tmp);
}

qse_lsp_obj_t* qse_lsp_prim_cdr (qse_lsp_t* lsp, qse_lsp_obj_t* args)
{
	/*
	 * (cdr '(10 20 30))
	 */

	qse_lsp_obj_t* tmp;

	QSE_ASSERT (QSE_LSP_TYPE(args) == QSE_LSP_OBJ_CONS);

	tmp = qse_lsp_eval (lsp, QSE_LSP_CAR(args));
	if (tmp == QSE_NULL) return QSE_NULL;
	if (tmp == lsp->mem->nil) return lsp->mem->nil;

	if (QSE_LSP_TYPE(tmp) != QSE_LSP_OBJ_CONS) 
	{
		qse_lsp_seterror (lsp, QSE_LSP_EARGBAD, QSE_NULL, 0);
		return QSE_NULL;
	}

	return QSE_LSP_CDR(tmp);
}

qse_lsp_obj_t* qse_lsp_prim_cons (qse_lsp_t* lsp, qse_lsp_obj_t* args)
{
	/*
	 * (cons 10 20)
	 * (cons '(10 20) 30)
	 */

	qse_lsp_obj_t* car, * cdr, * cons;

	QSE_ASSERT (QSE_LSP_TYPE(args) == QSE_LSP_OBJ_CONS);

	car = qse_lsp_eval (lsp, QSE_LSP_CAR(args));
	if (car == QSE_NULL) return QSE_NULL;

	if (qse_lsp_pushtmp (lsp, car) == QSE_NULL) return QSE_NULL;

	cdr = qse_lsp_eval (lsp, QSE_LSP_CAR(QSE_LSP_CDR(args)));
	if (cdr == QSE_NULL) 
	{
		qse_lsp_poptmp (lsp); /* car */
		return QSE_NULL;
	}

	if (qse_lsp_pushtmp (lsp, cdr) == QSE_NULL) 
	{
		qse_lsp_poptmp (lsp); /* car */
		return QSE_NULL;
	}

	cons = qse_lsp_makecons (lsp->mem, car, cdr);
	if (cons == QSE_NULL) 
	{
		qse_lsp_poptmp (lsp); /* cdr */
		qse_lsp_poptmp (lsp); /* car */
		return QSE_NULL;
	}

	qse_lsp_poptmp (lsp); /* cdr */
	qse_lsp_poptmp (lsp); /* car */
	return cons;
}

qse_lsp_obj_t* qse_lsp_prim_length (qse_lsp_t* lsp, qse_lsp_obj_t* args)
{
	qse_lsp_obj_t* body, * tmp;
	qse_long_t len = 0;

	QSE_ASSERT (QSE_LSP_TYPE(args) == QSE_LSP_OBJ_CONS);

	body = args;
	while (QSE_LSP_TYPE(body) == QSE_LSP_OBJ_CONS) 
	{
		tmp = qse_lsp_eval (lsp, QSE_LSP_CAR(body));
		if (tmp == QSE_NULL) return QSE_NULL;

		if (QSE_LSP_TYPE(tmp) == QSE_LSP_OBJ_NIL) 
		{
			len = 0;
		}
		else if (QSE_LSP_TYPE(tmp) == QSE_LSP_OBJ_STR)
		{
			len = QSE_LSP_STRLEN(tmp);
		}
		else if (QSE_LSP_TYPE(tmp) == QSE_LSP_OBJ_SYM)
		{
			len = QSE_LSP_SYMLEN(tmp);
		}
		else if (QSE_LSP_TYPE(tmp) == QSE_LSP_OBJ_CONS) 
		{
			len = 0;
			do 
			{
				len++;
				tmp = QSE_LSP_CDR(tmp);
			} 
			while (QSE_LSP_TYPE(tmp) == QSE_LSP_OBJ_CONS);

			/* TODO: more flexible without the check below?
			 *       both of the following expression evalute
			 *       to 3 without it.
			 *          (length '(9 9 9 . 9))
			 *          (length '(9 9 9))
			 */
			if (QSE_LSP_TYPE(tmp) != QSE_LSP_OBJ_NIL)
			{
				qse_lsp_seterror (lsp, QSE_LSP_EVALBAD, QSE_NULL, 0);
				return QSE_NULL;
			}
		}
		else 
		{
			qse_lsp_seterror (lsp, QSE_LSP_EVALBAD, QSE_NULL, 0);
			return QSE_NULL;
		}

		body = QSE_LSP_CDR(body);
	}

	QSE_ASSERT (body == lsp->mem->nil);
	return qse_lsp_makeint (lsp->mem, len);
}

qse_lsp_obj_t* qse_lsp_prim_set (qse_lsp_t* lsp, qse_lsp_obj_t* args)
{
	/*
	 * (set 'flowers 'rose)
	 * (set flowers 20)
	 * rose
	 */

	qse_lsp_obj_t* p1, * p2;

	QSE_ASSERT (QSE_LSP_TYPE(args) == QSE_LSP_OBJ_CONS);

	p1 = qse_lsp_eval (lsp, QSE_LSP_CAR(args));
	if (p1 == QSE_NULL) return QSE_NULL;

	if (qse_lsp_pushtmp (lsp, p1) == QSE_NULL) return QSE_NULL;

	if (QSE_LSP_TYPE(p1) != QSE_LSP_OBJ_SYM) 
	{
		qse_lsp_poptmp (lsp); /* p1 */

		qse_lsp_seterror (lsp, QSE_LSP_EARGBAD, QSE_NULL, 0);
		return QSE_NULL;
	}

	p2 = qse_lsp_eval (lsp, QSE_LSP_CAR(QSE_LSP_CDR(args)));
	if (p2 == QSE_NULL) 
	{
		qse_lsp_poptmp (lsp); /* p1 */
		return QSE_NULL;
	}

	if (qse_lsp_pushtmp (lsp, p2) == QSE_NULL) 
	{
		qse_lsp_poptmp (lsp); /* p1 */
		return QSE_NULL;
	}

	if (qse_lsp_setvalue (lsp->mem, p1, p2) == QSE_NULL) 
	{
		qse_lsp_poptmp (lsp); /* p2 */
		qse_lsp_poptmp (lsp); /* p1 */
		return QSE_NULL;
	}

	qse_lsp_poptmp (lsp); /* p2 */
	qse_lsp_poptmp (lsp); /* p1 */
	return p2;
}

qse_lsp_obj_t* qse_lsp_prim_setq (qse_lsp_t* lsp, qse_lsp_obj_t* args)
{
	/*
	 * (setq x 10)
	 * (setq x "string")
	 */

	qse_lsp_obj_t* p = args, * p1, * p2 = lsp->mem->nil;

	while (p != lsp->mem->nil) 
	{
		QSE_ASSERT (QSE_LSP_TYPE(p) == QSE_LSP_OBJ_CONS);

		p1 = QSE_LSP_CAR(p);
		if (QSE_LSP_TYPE(p1) != QSE_LSP_OBJ_SYM) 
		{
			qse_lsp_seterror (lsp, QSE_LSP_EARGBAD, QSE_NULL, 0);
			return QSE_NULL;
		}

		if (QSE_LSP_TYPE(QSE_LSP_CDR(p)) != QSE_LSP_OBJ_CONS) 
		{
			lsp->errnum = QSE_LSP_EARGFEW;
			return QSE_NULL;
		}

		p2 = qse_lsp_eval (lsp, QSE_LSP_CAR(QSE_LSP_CDR(p)));
		if (p2 == QSE_NULL) return QSE_NULL;

		if (qse_lsp_pushtmp (lsp, p2) == QSE_NULL) return QSE_NULL;

		if (qse_lsp_setvalue (lsp->mem, p1, p2) == QSE_NULL) 
		{
			qse_lsp_poptmp (lsp);
			return QSE_NULL;
		}

		qse_lsp_poptmp (lsp);
		p = QSE_LSP_CDR(QSE_LSP_CDR(p));
	}

	return p2;
}

qse_lsp_obj_t* qse_lsp_prim_quote (qse_lsp_t* lsp, qse_lsp_obj_t* args)
{
	/*
	 * (quote (10 20 30 50))
	 */

	QSE_ASSERT (QSE_LSP_TYPE(args) == QSE_LSP_OBJ_CONS);
	return QSE_LSP_CAR(args);
}

qse_lsp_obj_t* qse_lsp_prim_defun (qse_lsp_t* lsp, qse_lsp_obj_t* args)
{
	/*
	 * (defun x (a b)  (+ a b 100))
	 * (x 40 50)
	 *
	 * (setq x (lambda (x y) (setq temp 10) (+ x y temp)))
	 * (x 40 50)
	 * temp 
	 */

	qse_lsp_obj_t* name, * fun;

	name = QSE_LSP_CAR(args);
	if (QSE_LSP_TYPE(name) != QSE_LSP_OBJ_SYM) 
	{
		qse_lsp_seterror (lsp, QSE_LSP_EARGBAD, QSE_NULL, 0);
		return QSE_NULL;
	}

	fun = qse_lsp_makefunc (lsp->mem, 
		QSE_LSP_CAR(QSE_LSP_CDR(args)), QSE_LSP_CDR(QSE_LSP_CDR(args)));
	if (fun == QSE_NULL) return QSE_NULL;

	if (qse_lsp_pushtmp (lsp, fun) == QSE_NULL) return QSE_NULL;

	if (qse_lsp_setfunc (lsp->mem, QSE_LSP_CAR(args), fun) == QSE_NULL) 
	{
		qse_lsp_poptmp (lsp);
		return QSE_NULL;
	}

	qse_lsp_poptmp (lsp);
	return fun;
}

qse_lsp_obj_t* qse_lsp_prim_demac (qse_lsp_t* lsp, qse_lsp_obj_t* args)
{
	/*
	 * (demac x (abc)  x y z)
	 * (setq x (macro (abc) x y z))
	 */

	qse_lsp_obj_t* name, * mac;

	name = QSE_LSP_CAR(args);
	if (QSE_LSP_TYPE(name) != QSE_LSP_OBJ_SYM) 
	{
		qse_lsp_seterror (lsp, QSE_LSP_EARGBAD, QSE_NULL, 0);
		return QSE_NULL;
	}

	mac = qse_lsp_makemacro (lsp->mem, 
		QSE_LSP_CAR(QSE_LSP_CDR(args)), QSE_LSP_CDR(QSE_LSP_CDR(args)));
	if (mac == QSE_NULL) return QSE_NULL;

	if (qse_lsp_pushtmp (lsp, mac) == QSE_NULL) return QSE_NULL;

	if (qse_lsp_setfunc (lsp->mem, QSE_LSP_CAR(args), mac) == QSE_NULL) 
	{
		qse_lsp_poptmp (lsp);
		return QSE_NULL;
	}

	qse_lsp_poptmp (lsp);
	return mac;
}

qse_lsp_obj_t* qse_lsp_prim_or (qse_lsp_t* lsp, qse_lsp_obj_t* args)
{
	/*
	 * (or 10 20 30 40)
	 * (or (= n 20) (= n 30))
	 */
	qse_lsp_obj_t* tmp;

/* TODO: this is wrong. redo the work */
	while (QSE_LSP_TYPE(args) == QSE_LSP_OBJ_CONS) 
	{
		tmp = qse_lsp_eval (lsp, QSE_LSP_CAR(args));
		if (tmp == QSE_NULL) return QSE_NULL;

		if (QSE_LSP_TYPE(args) == QSE_LSP_OBJ_INT)
		if (tmp != lsp->mem->nil) return lsp->mem->t;
		args = QSE_LSP_CDR(args);
	}

	return lsp->mem->nil;
}

