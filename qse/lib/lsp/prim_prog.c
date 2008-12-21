/*
 * $Id: prim_prog.c 337 2008-08-20 09:17:25Z baconevi $
 *
 * {License}
 */

#include "lsp.h"

qse_lsp_obj_t* qse_lsp_prim_prog1 (qse_lsp_t* lsp, qse_lsp_obj_t* args)
{
	/* (prog1 1 2 3) returns 1 */
	qse_lsp_obj_t* res = QSE_NULL, * tmp;

	/*while (args != lsp->mem->nil) {*/
	while (QSE_LSP_TYPE(args) == QSE_LSP_OBJ_CONS) 
	{
		tmp = qse_lsp_eval (lsp, QSE_LSP_CAR(args));
		if (tmp == QSE_NULL) return QSE_NULL;

		if (res == QSE_NULL) 
		{
			res = tmp;
			if (qse_lsp_pushtmp (lsp, res) == QSE_NULL)
			{
				return QSE_NULL;
			}
		}
		args = QSE_LSP_CDR(args);
	}

	if (res != QSE_NULL) qse_lsp_poptmp (lsp);
	return res;
}

qse_lsp_obj_t* qse_lsp_prim_progn (qse_lsp_t* lsp, qse_lsp_obj_t* args)
{
	/* (progn 1 2 3) returns 3 */

	qse_lsp_obj_t* res, * tmp;

	res = lsp->mem->nil;
	/*while (args != lsp->mem->nil) {*/
	while (QSE_LSP_TYPE(args) == QSE_LSP_OBJ_CONS) 
	{
		tmp = qse_lsp_eval (lsp, QSE_LSP_CAR(args));
		if (tmp == QSE_NULL) return QSE_NULL;

		res = tmp;
		args = QSE_LSP_CDR(args);
	}

	return res;
}
