/*
 * $Id: prim_let.c 337 2008-08-20 09:17:25Z baconevi $
 *
 * {License}
 */

#include "lsp.h"

/* 
 * (let ((variable value)
 *       (variable value)
 *       ...)
 *      body...)
 */

static qse_lsp_obj_t* __prim_let (
	qse_lsp_t* lsp, qse_lsp_obj_t* args, int sequential)
{
	qse_lsp_frame_t* frame;
	qse_lsp_obj_t* assoc;
	qse_lsp_obj_t* body;
	qse_lsp_obj_t* value;

	/* create a new frameq */
	frame = qse_lsp_newframe (lsp);
	if (frame == QSE_NULL) return QSE_NULL;
	/*frame->link = lsp->mem->frame;*/

	if (sequential) 
	{
		frame->link = lsp->mem->frame;
		lsp->mem->frame = frame;
	}
	else 
	{
		frame->link = lsp->mem->brooding_frame;
		lsp->mem->brooding_frame = frame;
	}

	assoc = QSE_LSP_CAR(args);

	/*while (assoc != lsp->mem->nil) {*/
	while (QSE_LSP_TYPE(assoc) == QSE_LSP_OBJ_CONS) 
	{
		qse_lsp_obj_t* ass = QSE_LSP_CAR(assoc);
		if (QSE_LSP_TYPE(ass) == QSE_LSP_OBJ_CONS) 
		{
			qse_lsp_obj_t* n = QSE_LSP_CAR(ass);
			qse_lsp_obj_t* v = QSE_LSP_CDR(ass);

			if (QSE_LSP_TYPE(n) != QSE_LSP_OBJ_SYM) 
			{
				if (sequential) lsp->mem->frame = frame->link;
				else lsp->mem->brooding_frame = frame->link;
				qse_lsp_freeframe (lsp, frame);

				qse_lsp_seterror (lsp, QSE_LSP_EARGBAD, QSE_NULL, 0);
				return QSE_NULL;
			}

			if (v != lsp->mem->nil) 
			{
				if (QSE_LSP_CDR(v) != lsp->mem->nil) 
				{
					if (sequential) lsp->mem->frame = frame->link;
					else lsp->mem->brooding_frame = frame->link;
					qse_lsp_freeframe (lsp, frame);

					qse_lsp_seterror (lsp, QSE_LSP_EARGMANY, QSE_NULL, 0);
					return QSE_NULL;
				}
				if ((v = qse_lsp_eval(lsp, QSE_LSP_CAR(v))) == QSE_NULL) 
				{
					if (sequential) lsp->mem->frame = frame->link;
					else lsp->mem->brooding_frame = frame->link;
					qse_lsp_freeframe (lsp, frame);
					return QSE_NULL;
				}
			}

			if (qse_lsp_lookupinframe (lsp, frame, n) != QSE_NULL) 
			{
				if (sequential) lsp->mem->frame = frame->link;
				else lsp->mem->brooding_frame = frame->link;
				qse_lsp_freeframe (lsp, frame);

				qse_lsp_seterror (lsp, QSE_LSP_EDUPFML, QSE_NULL, 0);
				return QSE_NULL;
			}
			if (qse_lsp_insvalueintoframe (lsp, frame, n, v) == QSE_NULL) 
			{
				if (sequential) lsp->mem->frame = frame->link;
				else lsp->mem->brooding_frame = frame->link;
				qse_lsp_freeframe (lsp, frame);
				return QSE_NULL;
			}
		}
		else if (QSE_LSP_TYPE(ass) == QSE_LSP_OBJ_SYM) 
		{
			if (qse_lsp_lookupinframe (lsp, frame, ass) != QSE_NULL)
			{
				if (sequential) lsp->mem->frame = frame->link;
				else lsp->mem->brooding_frame = frame->link;
				qse_lsp_freeframe (lsp, frame);

				qse_lsp_seterror (lsp, QSE_LSP_EDUPFML, QSE_NULL, 0);
				return QSE_NULL;
			}
			if (qse_lsp_insvalueintoframe (lsp, frame, ass, lsp->mem->nil) == QSE_NULL) 
			{
				if (sequential) lsp->mem->frame = frame->link;
				else lsp->mem->brooding_frame = frame->link;
				qse_lsp_freeframe (lsp, frame);
				return QSE_NULL;
			}
		}
		else 
		{
			if (sequential) lsp->mem->frame = frame->link;
			else lsp->mem->brooding_frame = frame->link;
			qse_lsp_freeframe (lsp, frame);

			qse_lsp_seterror (lsp, QSE_LSP_EARGBAD, QSE_NULL, 0);
			return QSE_NULL;
		}

		assoc = QSE_LSP_CDR(assoc);
	}

	if (assoc != lsp->mem->nil) 
	{
		if (sequential) lsp->mem->frame = frame->link;
		else lsp->mem->brooding_frame = frame->link;
		qse_lsp_freeframe (lsp, frame);

		qse_lsp_seterror (lsp, QSE_LSP_EARGBAD, QSE_NULL, 0);
		return QSE_NULL;
	}

	/* push the frame */
	if (!sequential) 
	{
		lsp->mem->brooding_frame = frame->link;
		frame->link = lsp->mem->frame;
		lsp->mem->frame = frame;
	}

	/* evaluate forms in the body */
	value = lsp->mem->nil;
	body = QSE_LSP_CDR(args);
	while (body != lsp->mem->nil) 
	{
		value = qse_lsp_eval (lsp, QSE_LSP_CAR(body));
		if (value == QSE_NULL) 
		{
			lsp->mem->frame = frame->link;
			qse_lsp_freeframe (lsp, frame);
			return QSE_NULL;
		}
		body = QSE_LSP_CDR(body);
	}

	/* pop the frame */
	lsp->mem->frame = frame->link;

	/* destroy the frame */
	qse_lsp_freeframe (lsp, frame);
	return value;
}

qse_lsp_obj_t* qse_lsp_prim_let (qse_lsp_t* lsp, qse_lsp_obj_t* args)
{
	/*
	 * (defun x (x y) 
	 *     (let ((temp1 10) (temp2 20)) 
	 *          (+ x y temp1 temp2)))
	 * (x 40 50)
	 * temp1 
	 */
	return __prim_let (lsp, args, 0);
}

qse_lsp_obj_t* qse_lsp_prim_letx (qse_lsp_t* lsp, qse_lsp_obj_t* args)
{
	return __prim_let (lsp, args, 1);
}
