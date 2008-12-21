/*
 * $Id: env.c 337 2008-08-20 09:17:25Z baconevi $
 *
 * {License}
 */

#include "lsp.h"

/* TODO: make the frame hash accessible */

static qse_lsp_assoc_t* __new_assoc (
	qse_lsp_t* lsp, qse_lsp_obj_t* name, 
	qse_lsp_obj_t* value, qse_lsp_obj_t* func)
{
	qse_lsp_assoc_t* assoc;

	assoc = (qse_lsp_assoc_t*) 
		QSE_LSP_ALLOC (lsp, sizeof(qse_lsp_assoc_t));
	if (assoc == QSE_NULL) 
	{
		qse_lsp_seterror (lsp, QSE_LSP_ENOMEM, QSE_NULL, 0);
		return QSE_NULL;
	}

	assoc->name  = name;
	assoc->value = value;
	assoc->func  = func;
	assoc->link  = QSE_NULL;

	return assoc;
}

qse_lsp_frame_t* qse_lsp_newframe (qse_lsp_t* lsp)
{
	qse_lsp_frame_t* frame;

	frame = (qse_lsp_frame_t*) 
		QSE_LSP_ALLOC (lsp, sizeof(qse_lsp_frame_t));
	if (frame == QSE_NULL) 
	{
		qse_lsp_seterror (lsp, QSE_LSP_ENOMEM, QSE_NULL, 0);
		return QSE_NULL;
	}

	frame->assoc = QSE_NULL;
	frame->link  = QSE_NULL;

	return frame;
}

void qse_lsp_freeframe (qse_lsp_t* lsp, qse_lsp_frame_t* frame)
{
	qse_lsp_assoc_t* assoc, * link;

	/* destroy the associations */
	assoc = frame->assoc;
	while (assoc != QSE_NULL) 
	{
		link = assoc->link;
		QSE_LSP_FREE (lsp, assoc);
		assoc = link;
	}

	QSE_LSP_FREE (lsp, frame);
}

qse_lsp_assoc_t* qse_lsp_lookupinframe (
	qse_lsp_t* lsp, qse_lsp_frame_t* frame, qse_lsp_obj_t* name)
{
	qse_lsp_assoc_t* assoc;

	QSE_ASSERT (QSE_LSP_TYPE(name) == QSE_LSP_OBJ_SYM);

	assoc = frame->assoc;
	while (assoc != QSE_NULL) 
	{
		if (name == assoc->name) return assoc;
		assoc = assoc->link;
	}
	return QSE_NULL;
}

qse_lsp_assoc_t* qse_lsp_insvalueintoframe (
	qse_lsp_t* lsp, qse_lsp_frame_t* frame, 
	qse_lsp_obj_t* name, qse_lsp_obj_t* value)
{
	qse_lsp_assoc_t* assoc;

	QSE_ASSERT (QSE_LSP_TYPE(name) == QSE_LSP_OBJ_SYM);

	assoc = __new_assoc (lsp, name, value, QSE_NULL);
	if (assoc == QSE_NULL) return QSE_NULL;

	assoc->link  = frame->assoc;
	frame->assoc = assoc;
	return assoc;
}

qse_lsp_assoc_t* qse_lsp_insfuncintoframe (
	qse_lsp_t* lsp, qse_lsp_frame_t* frame, 
	qse_lsp_obj_t* name, qse_lsp_obj_t* func)
{
	qse_lsp_assoc_t* assoc;

	QSE_ASSERT (QSE_LSP_TYPE(name) == QSE_LSP_OBJ_SYM);

	assoc = __new_assoc (lsp, name, QSE_NULL, func);
	if (assoc == QSE_NULL) return QSE_NULL;

	assoc->link  = frame->assoc;
	frame->assoc = assoc;
	return assoc;
}

qse_lsp_tlink_t* qse_lsp_pushtmp (qse_lsp_t* lsp, qse_lsp_obj_t* obj)
{
	qse_lsp_tlink_t* tlink;

	tlink = (qse_lsp_tlink_t*) 
		QSE_LSP_ALLOC (lsp, sizeof(qse_lsp_tlink_t));
	if (tlink == QSE_NULL) 
	{
		qse_lsp_seterror (lsp, QSE_LSP_ENOMEM, QSE_NULL, 0);
		return QSE_NULL;
	}

	tlink->obj = obj;
	tlink->link = lsp->mem->tlink;
	lsp->mem->tlink = tlink;
	lsp->mem->tlink_count++;

	return tlink;
}

void qse_lsp_poptmp (qse_lsp_t* lsp)
{
	qse_lsp_tlink_t* top;

	QSE_ASSERT (lsp->mem->tlink != QSE_NULL);

	top = lsp->mem->tlink;
	lsp->mem->tlink = top->link;
	lsp->mem->tlink_count--;

	QSE_LSP_FREE (lsp, top);
}
