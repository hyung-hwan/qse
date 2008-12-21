/*
 * $Id: print.c 467 2008-12-09 09:55:51Z baconevi $
 *
 * {License}
 */

#include "lsp.h"

#define OUTPUT_STR(lsp,str) \
	do { \
		if (lsp->output_func(QSE_LSP_IO_WRITE, lsp->output_arg, (qse_char_t*)str, qse_strlen(str)) == -1) { \
			qse_lsp_seterror (lsp, QSE_LSP_EOUTPUT, QSE_NULL, 0); \
			return -1; \
		} \
	} while (0)

#define OUTPUT_STRX(lsp,str,len) \
	do { \
		if (lsp->output_func(QSE_LSP_IO_WRITE, lsp->output_arg, (qse_char_t*)str, len) == -1) { \
			qse_lsp_seterror (lsp, QSE_LSP_EOUTPUT, QSE_NULL, 0); \
			return -1; \
		} \
	} while (0)

static int __print (qse_lsp_t* lsp, const qse_lsp_obj_t* obj, qse_bool_t prt_cons_par)
{
	qse_char_t buf[256];

	if (lsp->output_func == QSE_NULL) 
	{
		qse_lsp_seterror (lsp, QSE_LSP_ENOOUTP, QSE_NULL, 0);
		return -1;
	}

	switch (QSE_LSP_TYPE(obj)) 
	{
		case QSE_LSP_OBJ_NIL:
			OUTPUT_STR (lsp, QSE_T("nil"));
			break;

		case QSE_LSP_OBJ_TRUE:
			OUTPUT_STR (lsp, QSE_T("t"));
			break;

		case QSE_LSP_OBJ_INT:
		#if QSE_SIZEOF_LONG_LONG > 0
			lsp->prmfns.misc.sprintf (
				lsp->prmfns.misc.data,
				buf, QSE_COUNTOF(buf), 
				QSE_T("%lld"), (long long)QSE_LSP_IVAL(obj));
		#elif QSE_SIZEOF___INT64 > 0
			lsp->prmfns.misc.sprintf (
				lsp->prmfns.misc.data,
				buf, QSE_COUNTOF(buf), 
				QSE_T("%I64d"), (__int64)QSE_LSP_IVAL(obj));
		#elif QSE_SIZEOF_LONG > 0
			lsp->prmfns.misc.sprintf (
				lsp->prmfns.misc.data,
				buf, QSE_COUNTOF(buf), 
				QSE_T("%ld"), (long)QSE_LSP_IVAL(obj));
		#elif QSE_SIZEOF_INT > 0
			lsp->prmfns.misc.sprintf (
				lsp->prmfns.misc.data,
				buf, QSE_COUNTOF(buf), 
				QSE_T("%d"), (int)QSE_LSP_IVAL(obj));
		#else
			#error unsupported size		
		#endif
			OUTPUT_STR (lsp, buf);
			break;

		case QSE_LSP_OBJ_REAL:
			lsp->prmfns.misc.sprintf (
				lsp->prmfns.misc.data,
				buf, QSE_COUNTOF(buf), 
				QSE_T("%Lf"), 
			#ifdef __MINGW32__
				(double)QSE_LSP_RVAL(obj)
			#else
				(long double)QSE_LSP_RVAL(obj)
			#endif
			);

			OUTPUT_STR (lsp, buf);
			break;

		case QSE_LSP_OBJ_SYM:
			OUTPUT_STRX (lsp, QSE_LSP_SYMPTR(obj), QSE_LSP_SYMLEN(obj));
			break;

		case QSE_LSP_OBJ_STR:
			OUTPUT_STR (lsp, QSE_T("\""));
			/* TODO: deescaping */
			OUTPUT_STRX (lsp, QSE_LSP_STRPTR(obj), QSE_LSP_STRLEN(obj));
			OUTPUT_STR (lsp, QSE_T("\""));
			break;

		case QSE_LSP_OBJ_CONS:
		{
			const qse_lsp_obj_t* p = obj;
			if (prt_cons_par) OUTPUT_STR (lsp, QSE_T("("));
			do 
			{
				qse_lsp_print (lsp, QSE_LSP_CAR(p));
				p = QSE_LSP_CDR(p);
				if (p != lsp->mem->nil) 
				{
					OUTPUT_STR (lsp, QSE_T(" "));
					if (QSE_LSP_TYPE(p) != QSE_LSP_OBJ_CONS) 
					{
						OUTPUT_STR (lsp, QSE_T(". "));
						qse_lsp_print (lsp, p);
					}
				}
			} 
			while (p != lsp->mem->nil && QSE_LSP_TYPE(p) == QSE_LSP_OBJ_CONS);
			if (prt_cons_par) OUTPUT_STR (lsp, QSE_T(")"));

			break;
		}

		case QSE_LSP_OBJ_FUNC:
			/*OUTPUT_STR (lsp, QSE_T("func"));*/
			OUTPUT_STR (lsp, QSE_T("(lambda "));
			if (__print (lsp, QSE_LSP_FFORMAL(obj), QSE_TRUE) == -1) return -1;
			OUTPUT_STR (lsp, QSE_T(" "));
			if (__print (lsp, QSE_LSP_FBODY(obj), QSE_FALSE) == -1) return -1;
			OUTPUT_STR (lsp, QSE_T(")"));
			break;

		case QSE_LSP_OBJ_MACRO:
			OUTPUT_STR (lsp, QSE_T("(macro "));
			if (__print (lsp, QSE_LSP_FFORMAL(obj), QSE_TRUE) == -1) return -1;
			OUTPUT_STR (lsp, QSE_T(" "));
			if (__print (lsp, QSE_LSP_FBODY(obj), QSE_FALSE) == -1) return -1;
			OUTPUT_STR (lsp, QSE_T(")"));
			break;
		case QSE_LSP_OBJ_PRIM:
			OUTPUT_STR (lsp, QSE_T("prim"));
			break;

		default:
			lsp->prmfns.misc.sprintf (
				lsp->prmfns.misc.data,
				buf, QSE_COUNTOF(buf),
				QSE_T("unknown object type: %d"), QSE_LSP_TYPE(obj)); 
			OUTPUT_STR (lsp, buf);
	}

	return 0;
}

int qse_lsp_print (qse_lsp_t* lsp, const qse_lsp_obj_t* obj)
{
	return __print (lsp, obj, QSE_TRUE);
}
