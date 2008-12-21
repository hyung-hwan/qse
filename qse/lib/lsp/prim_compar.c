/*
 * $Id: prim_compar.c 337 2008-08-20 09:17:25Z baconevi $
 *
 * {License}
 */

#include "lsp.h"

#define PRIM_COMPAR(lsp,args,op)  \
{ \
	qse_lsp_obj_t* p1, * p2; \
	int res; \
	QSE_ASSERT (QSE_LSP_TYPE(args) == QSE_LSP_OBJ_CONS); \
\
	p1 = qse_lsp_eval (lsp, QSE_LSP_CAR(args)); \
	if (p1 == QSE_NULL) return QSE_NULL; \
	if (qse_lsp_pushtmp (lsp, p1) == QSE_NULL) return QSE_NULL; \
\
	p2 = qse_lsp_eval (lsp, QSE_LSP_CAR(QSE_LSP_CDR(args))); \
	if (p2 == QSE_NULL) \
	{ \
		qse_lsp_poptmp (lsp); \
		return QSE_NULL; \
	} \
\
	if (qse_lsp_pushtmp (lsp, p2) == QSE_NULL) \
	{ \
		qse_lsp_poptmp (lsp); \
		return QSE_NULL; \
	} \
\
	if (QSE_LSP_TYPE(p1) == QSE_LSP_OBJ_INT) \
	{ \
		if (QSE_LSP_TYPE(p2) == QSE_LSP_OBJ_INT) \
		{ \
			res = QSE_LSP_IVAL(p1) op QSE_LSP_IVAL(p2); \
		} \
		else if (QSE_LSP_TYPE(p2) == QSE_LSP_OBJ_REAL) \
		{ \
			res = QSE_LSP_IVAL(p1) op QSE_LSP_RVAL(p2); \
		} \
		else \
		{ \
			qse_lsp_poptmp (lsp); \
			qse_lsp_poptmp (lsp); \
			qse_lsp_seterror (lsp, QSE_LSP_EVALBAD, QSE_NULL, 0); \
			return QSE_NULL; \
		} \
	} \
	else if (QSE_LSP_TYPE(p1) == QSE_LSP_OBJ_REAL) \
	{ \
		if (QSE_LSP_TYPE(p2) == QSE_LSP_OBJ_INT) \
		{ \
			res = QSE_LSP_RVAL(p1) op QSE_LSP_IVAL(p2); \
		} \
		else if (QSE_LSP_TYPE(p2) == QSE_LSP_OBJ_REAL) \
		{ \
			res = QSE_LSP_RVAL(p1) op QSE_LSP_RVAL(p2); \
		} \
		else \
		{ \
			qse_lsp_poptmp (lsp); \
			qse_lsp_poptmp (lsp); \
			qse_lsp_seterror (lsp, QSE_LSP_EVALBAD, QSE_NULL, 0); \
			return QSE_NULL; \
		} \
	} \
	else if (QSE_LSP_TYPE(p1) == QSE_LSP_OBJ_SYM) \
	{ \
		if (QSE_LSP_TYPE(p2) == QSE_LSP_OBJ_SYM) \
		{ \
			res = qse_strxncmp ( \
				QSE_LSP_SYMPTR(p1), QSE_LSP_SYMLEN(p1), \
				QSE_LSP_SYMPTR(p2), QSE_LSP_SYMLEN(p2)) op 0; \
		} \
		else  \
		{ \
			qse_lsp_poptmp (lsp); \
			qse_lsp_poptmp (lsp); \
			qse_lsp_seterror (lsp, QSE_LSP_EVALBAD, QSE_NULL, 0); \
			return QSE_NULL; \
		} \
	} \
	else if (QSE_LSP_TYPE(p1) == QSE_LSP_OBJ_STR) \
	{ \
		if (QSE_LSP_TYPE(p2) == QSE_LSP_OBJ_STR) \
		{ \
			res = qse_strxncmp ( \
				QSE_LSP_STRPTR(p1), QSE_LSP_STRLEN(p1),	\
				QSE_LSP_STRPTR(p2), QSE_LSP_STRLEN(p2)) op 0; \
		} \
		else \
		{ \
			qse_lsp_poptmp (lsp); \
			qse_lsp_poptmp (lsp); \
			qse_lsp_seterror (lsp, QSE_LSP_EVALBAD, QSE_NULL, 0); \
			return QSE_NULL; \
		} \
	} \
	else \
	{ \
		qse_lsp_poptmp (lsp); \
		qse_lsp_poptmp (lsp); \
		qse_lsp_seterror (lsp, QSE_LSP_EVALBAD, QSE_NULL, 0); \
		return QSE_NULL; \
	} \
\
	qse_lsp_poptmp (lsp); \
	qse_lsp_poptmp (lsp); \
	return (res)? lsp->mem->t: lsp->mem->nil; \
}

qse_lsp_obj_t* qse_lsp_prim_eq (qse_lsp_t* lsp, qse_lsp_obj_t* args)
{
	PRIM_COMPAR (lsp, args, ==);
}

qse_lsp_obj_t* qse_lsp_prim_ne (qse_lsp_t* lsp, qse_lsp_obj_t* args)
{
	PRIM_COMPAR (lsp, args, !=);
}

qse_lsp_obj_t* qse_lsp_prim_gt (qse_lsp_t* lsp, qse_lsp_obj_t* args)
{
	PRIM_COMPAR (lsp, args, >);
}

qse_lsp_obj_t* qse_lsp_prim_lt (qse_lsp_t* lsp, qse_lsp_obj_t* args)
{
	PRIM_COMPAR (lsp, args, <);
}

qse_lsp_obj_t* qse_lsp_prim_ge (qse_lsp_t* lsp, qse_lsp_obj_t* args)
{
	PRIM_COMPAR (lsp, args, >=);
}

qse_lsp_obj_t* qse_lsp_prim_le (qse_lsp_t* lsp, qse_lsp_obj_t* args)
{
	PRIM_COMPAR (lsp, args, <=);
}
