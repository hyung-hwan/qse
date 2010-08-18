/*
 * $Id: eval.c 337 2008-08-20 09:17:25Z baconevi $
 *
    Copyright 2006-2009 Chung, Hyung-Hwan.
    This file is part of QSE.

    QSE is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as 
    published by the Free Software Foundation, either version 3 of 
    the License, or (at your option) any later version.

    QSE is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public 
    License along with QSE. If not, see <http://www.gnu.org/licenses/>.
 */

#include "lsp.h"

static qse_lsp_obj_t* makefn (
	qse_lsp_t* lsp, qse_lsp_obj_t* cdr, int is_macro);
static qse_lsp_obj_t* eval_cons (
	qse_lsp_t* lsp, qse_lsp_obj_t* cons);
static qse_lsp_obj_t* apply (
	qse_lsp_t* lsp, qse_lsp_obj_t* func, qse_lsp_obj_t* actual);
static qse_lsp_obj_t* apply_to_prim (
	qse_lsp_t* lsp, qse_lsp_obj_t* func, qse_lsp_obj_t* actual);

qse_lsp_obj_t* qse_lsp_eval (qse_lsp_t* lsp, qse_lsp_obj_t* obj)
{
	if (QSE_LSP_TYPE(obj) == QSE_LSP_OBJ_CONS) 
	{
		return eval_cons (lsp, obj);
	}
	else if (QSE_LSP_TYPE(obj) == QSE_LSP_OBJ_SYM) 
	{
		qse_lsp_assoc_t* assoc; 

		/*
		if (obj == lsp->mem->lambda || obj == lsp->mem->macro) {
			qse_char_t* arg[1];

			arg[0] = QSE_LSP_SYMPTR(obj);

			printf ("lambda or macro can't be used as a normal symbol\n");
			qse_lsp_seterror (
				lsp, QSE_LSP_EBADSYM, 
				arg, QSE_COUNTOF(arg));
			return QSE_NULL;
		}
		*/

		assoc = qse_lsp_lookup(lsp->mem, obj);
		if (assoc == QSE_NULL || assoc->value == QSE_NULL) 
		{
			if (lsp->opt_undef_symbol) 
			{
				qse_cstr_t errarg;

          		errarg.len = QSE_LSP_SYMLEN(obj);
          		errarg.ptr = QSE_LSP_SYMPTR(obj);

				qse_lsp_seterror (lsp, QSE_LSP_EUNDEFSYM, &errarg, QSE_NULL);
				return QSE_NULL;
			}
			return lsp->mem->nil;
		}

		obj = assoc->value;
	}

	return obj;
}

static qse_lsp_obj_t* makefn (qse_lsp_t* lsp, qse_lsp_obj_t* cdr, int is_macro)
{
	qse_lsp_obj_t* func, * formal, * body, * p;

	if (cdr == lsp->mem->nil) 
	{
		qse_lsp_seterror (lsp, QSE_LSP_EARGFEW, QSE_NULL, QSE_NULL);
		return QSE_NULL;
	}

	if (QSE_LSP_TYPE(cdr) != QSE_LSP_OBJ_CONS) 
	{
		qse_lsp_seterror (lsp, QSE_LSP_EARGBAD, QSE_NULL, QSE_NULL);
		return QSE_NULL;
	}

	formal = QSE_LSP_CAR(cdr);
	body = QSE_LSP_CDR(cdr);

	if (body == lsp->mem->nil) 
	{
		qse_lsp_seterror (lsp, QSE_LSP_EEMPBDY, QSE_NULL, QSE_NULL);
		return QSE_NULL;
	}

/* TODO: more lambda expression syntax checks required???. */

	/* check if the lambda express has non-nil value 
	 * at the terminating cdr */
	for (p = body; QSE_LSP_TYPE(p) == QSE_LSP_OBJ_CONS; p = QSE_LSP_CDR(p));
	if (p != lsp->mem->nil) 
	{
		/* like in (lambda (x) (+ x 10) . 4) */
		qse_lsp_seterror (lsp, QSE_LSP_EARGBAD, QSE_NULL, QSE_NULL);
		return QSE_NULL;
	}

	func = (is_macro)?
		qse_lsp_makemacro (lsp->mem, formal, body):
		qse_lsp_makefunc (lsp->mem, formal, body);
	if (func == QSE_NULL) return QSE_NULL;

	return func;
}

static qse_lsp_obj_t* eval_cons (qse_lsp_t* lsp, qse_lsp_obj_t* cons)
{
	qse_lsp_obj_t* car, * cdr;
   
	QSE_ASSERT (QSE_LSP_TYPE(cons) == QSE_LSP_OBJ_CONS);

	car = QSE_LSP_CAR(cons);
	cdr = QSE_LSP_CDR(cons);

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
	else if (QSE_LSP_TYPE(car) == QSE_LSP_OBJ_SYM) 
	{
		qse_lsp_assoc_t* assoc;

		if ((assoc = qse_lsp_lookup(lsp->mem, car)) != QSE_NULL) 
		{
			/*qse_lsp_obj_t* func = assoc->value;*/
			qse_lsp_obj_t* func = assoc->func;
			if (func == QSE_NULL) 
			{
				/* the symbol's function definition is void */
				qse_cstr_t errarg;

          		errarg.len = QSE_LSP_SYMLEN(car);
          		errarg.ptr = QSE_LSP_SYMPTR(car);

				qse_lsp_seterror (lsp, QSE_LSP_EUNDEFFN, &errarg, QSE_NULL);
				return QSE_NULL;
			}

			if (QSE_LSP_TYPE(func) == QSE_LSP_OBJ_FUNC ||
			    QSE_LSP_TYPE(func) == QSE_LSP_OBJ_MACRO) 
			{
				return apply (lsp, func, cdr);
			}
			else if (QSE_LSP_TYPE(func) == QSE_LSP_OBJ_PRIM) 
			{
				/* primitive function */
				return apply_to_prim (lsp, func, cdr);
			}
			else 
			{
				qse_cstr_t errarg;

          		errarg.len = QSE_LSP_SYMLEN(car);
          		errarg.ptr = QSE_LSP_SYMPTR(car);

				qse_lsp_seterror (lsp, QSE_LSP_EUNDEFFN, &errarg, QSE_NULL);
				return QSE_NULL;
			}
		}
		else 
		{
			qse_cstr_t errarg;

          	errarg.len = QSE_LSP_SYMLEN(car);
          	errarg.ptr = QSE_LSP_SYMPTR(car);

			qse_lsp_seterror (lsp, QSE_LSP_EUNDEFFN, &errarg, QSE_NULL);
			return QSE_NULL;
		}
	}
	else if (QSE_LSP_TYPE(car) == QSE_LSP_OBJ_FUNC || 
	         QSE_LSP_TYPE(car) == QSE_LSP_OBJ_MACRO) 
	{
		return apply (lsp, car, cdr);
	}
	else if (QSE_LSP_TYPE(car) == QSE_LSP_OBJ_CONS) 
	{
		/* anonymous function or macros 
		 * ((lambda (x) (+ x 10)) 50) */
		if (QSE_LSP_CAR(car) == lsp->mem->lambda) 
		{
			qse_lsp_obj_t* func = makefn (lsp, QSE_LSP_CDR(car), 0);
			if (func == QSE_NULL) return QSE_NULL;
			return apply (lsp, func, cdr);
		}
		else if (QSE_LSP_CAR(car) == lsp->mem->macro) 
		{
			qse_lsp_obj_t* func = makefn (lsp, QSE_LSP_CDR(car), 1);
			if (func == QSE_NULL) return QSE_NULL;
			return apply (lsp, func, cdr);
		}
	}

	qse_lsp_seterror (lsp, QSE_LSP_EBADFN, QSE_NULL, QSE_NULL);
	return QSE_NULL;
}

static qse_lsp_obj_t* apply (
	qse_lsp_t* lsp, qse_lsp_obj_t* func, qse_lsp_obj_t* actual)
{
	qse_lsp_frame_t* frame;
	qse_lsp_obj_t* formal;
	qse_lsp_obj_t* body;
	qse_lsp_obj_t* value;
	qse_lsp_mem_t* mem;

	QSE_ASSERT (QSE_LSP_TYPE(func) == QSE_LSP_OBJ_FUNC ||
	            QSE_LSP_TYPE(func) == QSE_LSP_OBJ_MACRO);

	QSE_ASSERT (QSE_LSP_TYPE(QSE_LSP_CDR(func)) == QSE_LSP_OBJ_CONS);

	mem = lsp->mem;

	if (QSE_LSP_TYPE(func) == QSE_LSP_OBJ_MACRO) 
	{
		formal = QSE_LSP_MFORMAL (func);
		body   = QSE_LSP_MBODY   (func);
	}
	else 
	{
		formal = QSE_LSP_FFORMAL (func);
		body   = QSE_LSP_FBODY   (func);
	}

	/* make a new frame. */
	frame = qse_lsp_newframe (lsp);
	if (frame == QSE_NULL) return QSE_NULL;

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
			qse_lsp_freeframe (lsp, frame);

			qse_lsp_seterror (lsp, QSE_LSP_EARGFEW, QSE_NULL, QSE_NULL);
			return QSE_NULL;
		}

		value = QSE_LSP_CAR(actual);
		if (QSE_LSP_TYPE(func) != QSE_LSP_OBJ_MACRO) 
		{
			/* macro doesn't evaluate actual arguments. */
			value = qse_lsp_eval (lsp, value);
			if (value == QSE_NULL) 
			{
				mem->brooding_frame = frame->link;
				qse_lsp_freeframe (lsp, frame);
				return QSE_NULL;
			}
		}

		if (qse_lsp_lookupinframe (
			lsp, frame, QSE_LSP_CAR(formal)) != QSE_NULL) 
		{
			mem->brooding_frame = frame->link;
			qse_lsp_freeframe (lsp, frame);

			qse_lsp_seterror (lsp, QSE_LSP_EDUPFML, QSE_NULL, QSE_NULL);
			return QSE_NULL;
		}

		if (qse_lsp_insvalueintoframe (
			lsp, frame, QSE_LSP_CAR(formal), value) == QSE_NULL) 
		{
			mem->brooding_frame = frame->link;
			qse_lsp_freeframe (lsp, frame);
			return QSE_NULL;
		}

		actual = QSE_LSP_CDR(actual);
		formal = QSE_LSP_CDR(formal);
	}

	if (QSE_LSP_TYPE(actual) == QSE_LSP_OBJ_CONS) 
	{
		mem->brooding_frame = frame->link;
		qse_lsp_freeframe (lsp, frame);

		qse_lsp_seterror (lsp, QSE_LSP_EARGMANY, QSE_NULL, QSE_NULL);
		return QSE_NULL;
	}
	else if (actual != mem->nil) 
	{
		mem->brooding_frame = frame->link;
		qse_lsp_freeframe (lsp, frame);

		qse_lsp_seterror (lsp, QSE_LSP_EARGBAD, QSE_NULL, QSE_NULL);
		return QSE_NULL;
	}

	/* push the frame */
	mem->brooding_frame = frame->link;
	frame->link = mem->frame;
	mem->frame = frame;

	/* do the evaluation of the body */
	value = mem->nil;
	while (body != mem->nil) 
	{
		value = qse_lsp_eval(lsp, QSE_LSP_CAR(body));
		if (value == QSE_NULL) 
		{
			mem->frame = frame->link;
			qse_lsp_freeframe (lsp, frame);
			return QSE_NULL;
		}
		body = QSE_LSP_CDR(body);
	}

	/* pop the frame. */
	mem->frame = frame->link;

	/* destroy the frame. */
	qse_lsp_freeframe (lsp, frame);

	/*if (QSE_LSP_CAR(func) == mem->macro) {*/
	if (QSE_LSP_TYPE(func) == QSE_LSP_OBJ_MACRO) 
	{
		value = qse_lsp_eval (lsp, value);
		if (value == QSE_NULL) return QSE_NULL;
	}

	return value;
}

static qse_lsp_obj_t* apply_to_prim (
	qse_lsp_t* lsp, qse_lsp_obj_t* func, qse_lsp_obj_t* actual)
{
	qse_lsp_obj_t* obj;
	qse_size_t count = 0;

	QSE_ASSERT (QSE_LSP_TYPE(func) == QSE_LSP_OBJ_PRIM);

	obj = actual;
	while (QSE_LSP_TYPE(obj) == QSE_LSP_OBJ_CONS) 
	{
		count++;
		obj = QSE_LSP_CDR(obj);
	}	
	if (obj != lsp->mem->nil) 
	{
		qse_lsp_seterror (lsp, QSE_LSP_EARGBAD, QSE_NULL, QSE_NULL);
		return QSE_NULL;
	}

	if (count < QSE_LSP_PMINARGS(func))
	{
		qse_lsp_seterror (lsp, QSE_LSP_EARGFEW, QSE_NULL, QSE_NULL);
		return QSE_NULL;
	}

	if (count > QSE_LSP_PMAXARGS(func))
	{
		qse_lsp_seterror (lsp, QSE_LSP_EARGMANY, QSE_NULL, QSE_NULL);
		return QSE_NULL;
	} 

	return QSE_LSP_PIMPL(func) (lsp, actual);
}
