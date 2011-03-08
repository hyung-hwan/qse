/*
 * $Id$
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

#include "scm.h"

static int eval_entity (qse_scm_t* scm);

static int save (qse_scm_t* scm, qse_scm_ent_t* )
{
}

static int leave (qse_scm_t* scm)
{
}

int qse_scm_dolambda (qse_scm_t* scm)
{
	qse_scm_ent_t* obj;

	obj = qse_scm_makeclosent (scm, scm->e.cod, scm->e.env);
	if (obj == QSE_NULL) return -1;
	scm->e.out = obj;
	return 0;
}

int qse_scm_doquote (qse_scm_t* scm)
{
	/* For the expression (quote 10),
	 * 	scm.e.cod is (10).
	 * 	PAIR_CAR(scm.e.cod) is 10 
	 */
	scm->e.out = PAIR_CAR(scm->e.cod);
	return 0;
}

static int define_finish (qse_scm_t* scm)
{
	qse_scm_ent_t* var = scm->e.cod;
	//set var in the environemtn....	
	
	leave (scm);
	return 0;
}

int qse_scm_dodefine (qse_scm_t* scm)
{
	qse_scm_ent_t* car, * cdr;

	car = PAIR_CAR(scm->e.cod);
	cdr = PAIR_CDR(scm->e.cod);

/* TODO: support function defintion - (define (f x y) (+ x y) (* x y)) 
 -> support it by converting it to lambda expression
 -> (define f (lambda (x y) (+ x y) (* x y)) 
 */
	if (IS_SMALLINT(scm,cdr) || TYPE(cdr) != QSE_SCM_ENT_PAIR)
	{
		/* (define x . 10) */
		/* TODO: change error code ... */
		qse_scm_seterror (scm, QSE_SCM_EARGBAD, QSE_NULL, QSE_NULL);
		return -1;
	}

	if (!IS_NIL(scm,PAIR_CDR(cdr)))
	{
		/* (define x 10 . 20)
		 * (define x 10 20) */
		qse_scm_seterror (scm, QSE_SCM_EARGMANY, QSE_NULL, QSE_NULL);
		return -1;
	}
		
	if (IS_SMALLINT(scm,car) || TYPE(car) != QSE_SCM_ENT_SYM)
	{
		/* check if the variable is a symbol 
		 * (define 20 10)
		 */
		qse_scm_seterror (scm, QSE_SCM_EVARBAD, QSE_NULL, QSE_NULL);
		return -1;
	}

	save car...

// let it jump to EVAL and come back to DEFINE_FINISH...
	scm->e.cod = PAIR_CAR(cdr);
	scm->e.op = eval_entity;
	return 0;
}

int qse_scm_dobegin (qse_scm_t* scm)
{
	/*
	(begin
		(print "hello")
		(print "world")
	)
	*/

	qse_scm_ent_t* car, * cdr;
	
	if (IS_SMALLINT(scm.e.cod) || TYPE(scm.e.cod) != QSE_SCM_ENT_PAIR)
	{
		/* (begin (+ x y) . 30) */
		qse_scm_seterror (scm, QSE_SCM_EARGBAD, QSE_NULL, QSE_NULL);
		return -1;
	}

	car = PAIR_CAR(scm.e.cod);
	cdr = PAIR_CDR(scm.e.cod);
	
	if (!IS_NIL(cdr))
	{
save (BEGIN... cdr);
	}

	scm.e.cod = car;
	scm.e.op = eval_entity;
	return 0;
}

int qse_scm_doif (qse_scm_t* scm)
{
	return 0;
}

static int eval_entity (qse_scm_t* scm)
{
	if (IS_SMALLINT(scm,scm->e.cod)) 
	{
	}
	else if (TYPE(scm->e.cod) == QSE_SCM_ENT_PAIR)
	{
		qse_scm_ent_t* car, * cdr;

		/* the first item in the list */
		car = PAIR_CAR(scm->e.cod);
		if (SYNT(car))
		{
qse_printf (QSE_T("xxxxx\n"));
			/* the first item in the list is a syntax symbol */
			cdr = PAIR_CDR(scm->e.cod);
			if (IS_SMALLINT(scm,cdr) || TYPE(cdr) != QSE_SCM_ENT_PAIR)
			{
				/* check if the cdr part ends the list with a dot
				 * as in (quote . 10) */
				qse_scm_seterror (scm, QSE_SCM_EARGBAD, 0, 0);
				return -1;
			}

			/* go on to the syntax function */
			scm->e.cod = cdr;
			scm->e.op = SYNT_UPTR(car);
		}
		else
		{
			/*
			push E1_ARG.... NIL, PAIR_CDR(code)
			scm->e.cod = car;
			goback to eval...
			*/
		}
	}
	else if (TYPE(scm->e.cod) == QSE_SCM_ENT_SYM)
	{
		/* resolve the symbol from the environment */
	}
	else
	{
	}

	return 0;
}

qse_scm_ent_t* qse_scm_eval (qse_scm_t* scm, qse_scm_ent_t* obj)
{
	scm->e.dmp = scm->nil;
	scm->e.env = scm->gloenv;
	scm->e.cod = obj;

	scm->e.in = obj;
	scm->e.out = scm->nil;
	scm->e.op = eval_entity;

	do
	{
		if (scm->e.op (scm) <= -1) return QSE_NULL;
		break;
	}
	while (scm->e.op);

	return scm->e.out;
}
