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

static qse_scm_ent_t* apply (qse_scm_t* scm)
{
	if (TYPE(scm->reg.cod) == QSE_SCM_ENT_PROC)
	{
		/* builtin-procedure */
	}	
#if 0
	else if (TYPE(scm->reg.cod) == QSE_SCM_ENT_CLO)
	{
		/* closure */
	}	
	else if (TYPE(scm->reg.cod) == QSE_SCM_ENT_CON)
	{
		/* continuation */
	}
#endif
	else
	{
	}
}

static qse_scm_ent_t* eval_args (qse_scm_t* scm)
{
	args = cons (value, args);
	if (TYPE(scm->reg.cod))
	{
	}
	else
	{
	}
}

static qse_scm_ent_t* eval_entity (qse_scm_t* scm)
{

	if (IS_SMALLINT(scm->reg.cod)) 
	{
	}
	else if (TYPE(scm->reg.cod) == QSE_SCM_ENT_SYM)
	{
	}
	else if (TYPE(scm->reg.cod) == QSE_SCM_ENT_PAIR)
	{
		car = PAIR_CAR(scm->reg.cod);
		if (SYNT(car))
		{
		}
		else
		{
			push E1_ARG.... NIL, PAIR_CDR(code)
			scm->reg.cod = car;
			goback to eval...
		}
	}
	else
	{
	}
}

qse_scm_ent_t* qse_scm_eval (qse_scm_t* scm, qse_scm_ent_t* obj)
{
	scm->reg.dmp = scm->nil;
	scm->reg.env = scm->gloenv;
	scm->reg.cod = obj;

	return eval_entity (scm);
}
