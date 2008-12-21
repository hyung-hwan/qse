/*
 * $Id: prim_math.c 337 2008-08-20 09:17:25Z baconevi $
 *
 * {License}
 */

#include "lsp.h"

qse_lsp_obj_t* qse_lsp_prim_plus (qse_lsp_t* lsp, qse_lsp_obj_t* args)
{
	qse_lsp_obj_t* body, * tmp;
	qse_long_t ival = 0;
	qse_real_t rval = .0;
	qse_bool_t realnum = QSE_FALSE;

	QSE_ASSERT (QSE_LSP_TYPE(args) == QSE_LSP_OBJ_CONS);

	body = args;
	while (QSE_LSP_TYPE(body) == QSE_LSP_OBJ_CONS) 
	{
		tmp = qse_lsp_eval (lsp, QSE_LSP_CAR(body));
		if (tmp == QSE_NULL) return QSE_NULL;

		if (QSE_LSP_TYPE(tmp) == QSE_LSP_OBJ_INT) 
		{
			if (body == args) 
			{
				QSE_ASSERT (realnum == QSE_FALSE);
				ival = QSE_LSP_IVAL(tmp);
			}
			else 
			{
				if (!realnum) 
					ival = ival + QSE_LSP_IVAL(tmp);
				else
					rval = rval + QSE_LSP_IVAL(tmp);
			}
		}
		else if (QSE_LSP_TYPE(tmp) == QSE_LSP_OBJ_REAL) 
		{
			if (body == args) 
			{
				QSE_ASSERT (realnum == QSE_FALSE);
				realnum = QSE_TRUE;
				rval = QSE_LSP_RVAL(tmp);
			}
			else 
			{
				if (!realnum) 
				{
					realnum = QSE_TRUE;
					rval = (qse_real_t)ival;
				}
				rval = rval + QSE_LSP_RVAL(tmp);
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

	tmp = (realnum)?
		qse_lsp_makerealobj (lsp->mem, rval):
		qse_lsp_makeintobj (lsp->mem, ival);
	if (tmp == QSE_NULL) return QSE_NULL;

	return tmp;
}

qse_lsp_obj_t* qse_lsp_prim_minus (qse_lsp_t* lsp, qse_lsp_obj_t* args)
{
	qse_lsp_obj_t* body, * tmp;
	qse_long_t ival = 0;
	qse_real_t rval = .0;
	qse_bool_t realnum = QSE_FALSE;

	QSE_ASSERT (QSE_LSP_TYPE(args) == QSE_LSP_OBJ_CONS);

	body = args;
	while (QSE_LSP_TYPE(body) == QSE_LSP_OBJ_CONS) 
	{
		tmp = qse_lsp_eval (lsp, QSE_LSP_CAR(body));
		if (tmp == QSE_NULL) return QSE_NULL;


		if (QSE_LSP_TYPE(tmp) == QSE_LSP_OBJ_INT) 
		{
			if (body == args) 
			{
				QSE_ASSERT (realnum == QSE_FALSE);
				ival = QSE_LSP_IVAL(tmp);
			}
			else 
			{
				if (!realnum) 
					ival = ival - QSE_LSP_IVAL(tmp);
				else
					rval = rval - QSE_LSP_IVAL(tmp);
			}
		}
		else if (QSE_LSP_TYPE(tmp) == QSE_LSP_OBJ_REAL) 
		{
			if (body == args) 
			{
				QSE_ASSERT (realnum == QSE_FALSE);
				realnum = QSE_TRUE;
				rval = QSE_LSP_RVAL(tmp);
			}
			else 
			{
				if (!realnum) 
				{
					realnum = QSE_TRUE;
					rval = (qse_real_t)ival;
				}
				rval = rval - QSE_LSP_RVAL(tmp);
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

	tmp = (realnum)?
		qse_lsp_makerealobj (lsp->mem, rval):
		qse_lsp_makeintobj (lsp->mem, ival);
	if (tmp == QSE_NULL) return QSE_NULL;

	return tmp;
}

qse_lsp_obj_t* qse_lsp_prim_mul (qse_lsp_t* lsp, qse_lsp_obj_t* args)
{
	qse_lsp_obj_t* body, * tmp;
	qse_long_t ival = 0;
	qse_real_t rval = .0;
	qse_bool_t realnum = QSE_FALSE;

	QSE_ASSERT (QSE_LSP_TYPE(args) == QSE_LSP_OBJ_CONS);

	body = args;
	while (QSE_LSP_TYPE(body) == QSE_LSP_OBJ_CONS) 
	{
		tmp = qse_lsp_eval (lsp, QSE_LSP_CAR(body));
		if (tmp == QSE_NULL) return QSE_NULL;

		if (QSE_LSP_TYPE(tmp) == QSE_LSP_OBJ_INT) 
		{
			if (body == args) 
			{
				QSE_ASSERT (realnum == QSE_FALSE);
				ival = QSE_LSP_IVAL(tmp);
			}
			else 
			{
				if (!realnum) 
					ival = ival * QSE_LSP_IVAL(tmp);
				else
					rval = rval * QSE_LSP_IVAL(tmp);
			}
		}
		else if (QSE_LSP_TYPE(tmp) == QSE_LSP_OBJ_REAL) 
		{
			if (body == args) 
			{
				QSE_ASSERT (realnum == QSE_FALSE);
				realnum = QSE_TRUE;
				rval = QSE_LSP_RVAL(tmp);
			}
			else 
			{
				if (!realnum) 
				{
					realnum = QSE_TRUE;
					rval = (qse_real_t)ival;
				}
				rval = rval * QSE_LSP_RVAL(tmp);
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

	tmp = (realnum)?
		qse_lsp_makerealobj (lsp->mem, rval):
		qse_lsp_makeintobj (lsp->mem, ival);
	if (tmp == QSE_NULL) return QSE_NULL;

	return tmp;
}

qse_lsp_obj_t* qse_lsp_prim_div (qse_lsp_t* lsp, qse_lsp_obj_t* args)
{
	qse_lsp_obj_t* body, * tmp;
	qse_long_t ival = 0;
	qse_real_t rval = .0;
	qse_bool_t realnum = QSE_FALSE;

	QSE_ASSERT (QSE_LSP_TYPE(args) == QSE_LSP_OBJ_CONS);

	body = args;
	while (QSE_LSP_TYPE(body) == QSE_LSP_OBJ_CONS) 
	{
		tmp = qse_lsp_eval (lsp, QSE_LSP_CAR(body));
		if (tmp == QSE_NULL) return QSE_NULL;

		if (QSE_LSP_TYPE(tmp) == QSE_LSP_OBJ_INT) 
		{
			if (body == args) 
			{
				QSE_ASSERT (realnum == QSE_FALSE);
				ival = QSE_LSP_IVAL(tmp);
			}
			else 
			{
				if (!realnum) 
				{
					if (QSE_LSP_IVAL(tmp) == 0) 
					{
						qse_lsp_seterror (lsp, QSE_LSP_EDIVBY0, QSE_NULL, 0);
						return QSE_NULL;
					}
					ival = ival / QSE_LSP_IVAL(tmp);
				}
				else
					rval = rval / QSE_LSP_IVAL(tmp);
			}
		}
		else if (QSE_LSP_TYPE(tmp) == QSE_LSP_OBJ_REAL) 
		{
			if (body == args) 
			{
				QSE_ASSERT (realnum == QSE_FALSE);
				realnum = QSE_TRUE;
				rval = QSE_LSP_RVAL(tmp);
			}
			else 
			{
				if (!realnum) 
				{
					realnum = QSE_TRUE;
					rval = (qse_real_t)ival;
				}
				rval = rval / QSE_LSP_RVAL(tmp);
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

	tmp = (realnum)?
		qse_lsp_makerealobj (lsp->mem, rval):
		qse_lsp_makeintobj (lsp->mem, ival);
	if (tmp == QSE_NULL) return QSE_NULL;

	return tmp;
}

qse_lsp_obj_t* qse_lsp_prim_mod (qse_lsp_t* lsp, qse_lsp_obj_t* args)
{
	qse_lsp_obj_t* body, * tmp;
	qse_long_t ival = 0;

	QSE_ASSERT (QSE_LSP_TYPE(args) == QSE_LSP_OBJ_CONS);

	body = args;
	while (QSE_LSP_TYPE(body) == QSE_LSP_OBJ_CONS) 
	{
		tmp = qse_lsp_eval (lsp, QSE_LSP_CAR(body));
		if (tmp == QSE_NULL) return QSE_NULL;

		if (QSE_LSP_TYPE(tmp) == QSE_LSP_OBJ_INT) 
		{
			if (body == args) 
			{
				ival = QSE_LSP_IVAL(tmp);
			}
			else 
			{
				if (QSE_LSP_IVAL(tmp) == 0) 
				{
					qse_lsp_seterror (lsp, QSE_LSP_EDIVBY0, QSE_NULL, 0);
					return QSE_NULL;
				}
				ival = ival % QSE_LSP_IVAL(tmp);
			}
		}
		else if (QSE_LSP_TYPE(tmp) == QSE_LSP_OBJ_REAL) 
		{
			if (body == args) 
			{
				ival = (qse_long_t)QSE_LSP_RVAL(tmp);
			}
			else 
			{
				qse_long_t tmpi = (qse_long_t)QSE_LSP_RVAL(tmp);
				if (tmpi == 0) 
				{
					qse_lsp_seterror (lsp, QSE_LSP_EDIVBY0, QSE_NULL, 0);
					return QSE_NULL;
				}
				ival = ival % tmpi;
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

	tmp = qse_lsp_makeintobj (lsp->mem, ival);
	if (tmp == QSE_NULL) return QSE_NULL;

	return tmp;
}
