/*
 * $Id: print.c,v 1.1 2007/03/28 14:05:24 bacon Exp $
 *
 * {License}
 */

#include <ase/lsp/lsp_i.h>

#define OUTPUT_STR(lsp,str) \
	do { \
		if (lsp->output_func(ASE_LSP_IO_WRITE, lsp->output_arg, (ase_char_t*)str, ase_strlen(str)) == -1) { \
			ase_lsp_seterror (lsp, ASE_LSP_EOUTPUT, ASE_NULL, 0); \
			return -1; \
		} \
	} while (0)

#define OUTPUT_STRX(lsp,str,len) \
	do { \
		if (lsp->output_func(ASE_LSP_IO_WRITE, lsp->output_arg, (ase_char_t*)str, len) == -1) { \
			ase_lsp_seterror (lsp, ASE_LSP_EOUTPUT, ASE_NULL, 0); \
			return -1; \
		} \
	} while (0)

static int __print (ase_lsp_t* lsp, const ase_lsp_obj_t* obj, ase_bool_t prt_cons_par)
{
	ase_char_t buf[256];

	if (lsp->output_func == ASE_NULL) 
	{
		ase_lsp_seterror (lsp, ASE_LSP_ENOOUTP, ASE_NULL, 0);
		return -1;
	}

	switch (ASE_LSP_TYPE(obj)) 
	{
		case ASE_LSP_OBJ_NIL:
			OUTPUT_STR (lsp, ASE_T("nil"));
			break;

		case ASE_LSP_OBJ_TRUE:
			OUTPUT_STR (lsp, ASE_T("t"));
			break;

		case ASE_LSP_OBJ_INT:
		#if ASE_SIZEOF_LONG_LONG > 0
			lsp->prmfns.misc.sprintf (
				lsp->prmfns.misc.custom_data,
				buf, ASE_COUNTOF(buf), 
				ASE_T("%lld"), (long long)ASE_LSP_IVAL(obj));
		#elif ASE_SIZEOF___INT64 > 0
			lsp->prmfns.misc.sprintf (
				lsp->prmfns.misc.custom_data,
				buf, ASE_COUNTOF(buf), 
				ASE_T("%I64d"), (__int64)ASE_LSP_IVAL(obj));
		#elif ASE_SIZEOF_LONG > 0
			lsp->prmfns.misc.sprintf (
				lsp->prmfns.misc.custom_data,
				buf, ASE_COUNTOF(buf), 
				ASE_T("%ld"), (long)ASE_LSP_IVAL(obj));
		#elif ASE_SIZEOF_INT > 0
			lsp->prmfns.misc.sprintf (
				lsp->prmfns.misc.custom_data,
				buf, ASE_COUNTOF(buf), 
				ASE_T("%d"), (int)ASE_LSP_IVAL(obj));
		#else
			#error unsupported size		
		#endif
			OUTPUT_STR (lsp, buf);
			break;

		case ASE_LSP_OBJ_REAL:
			lsp->prmfns.misc.sprintf (
				lsp->prmfns.misc.custom_data,
				buf, ASE_COUNTOF(buf), 
				ASE_T("%Lf"), (long double)ASE_LSP_RVAL(obj));

			OUTPUT_STR (lsp, buf);
			break;

		case ASE_LSP_OBJ_SYM:
			OUTPUT_STRX (lsp, ASE_LSP_SYMPTR(obj), ASE_LSP_SYMLEN(obj));
			break;

		case ASE_LSP_OBJ_STR:
			OUTPUT_STR (lsp, ASE_T("\""));
			/* TODO: deescaping */
			OUTPUT_STRX (lsp, ASE_LSP_STRPTR(obj), ASE_LSP_STRLEN(obj));
			OUTPUT_STR (lsp, ASE_T("\""));
			break;

		case ASE_LSP_OBJ_CONS:
		{
			const ase_lsp_obj_t* p = obj;
			if (prt_cons_par) OUTPUT_STR (lsp, ASE_T("("));
			do 
			{
				ase_lsp_print (lsp, ASE_LSP_CAR(p));
				p = ASE_LSP_CDR(p);
				if (p != lsp->mem->nil) 
				{
					OUTPUT_STR (lsp, ASE_T(" "));
					if (ASE_LSP_TYPE(p) != ASE_LSP_OBJ_CONS) 
					{
						OUTPUT_STR (lsp, ASE_T(". "));
						ase_lsp_print (lsp, p);
					}
				}
			} 
			while (p != lsp->mem->nil && ASE_LSP_TYPE(p) == ASE_LSP_OBJ_CONS);
			if (prt_cons_par) OUTPUT_STR (lsp, ASE_T(")"));

			break;
		}

		case ASE_LSP_OBJ_FUNC:
			/*OUTPUT_STR (lsp, ASE_T("func"));*/
			OUTPUT_STR (lsp, ASE_T("(lambda "));
			if (__print (lsp, ASE_LSP_FFORMAL(obj), ase_true) == -1) return -1;
			OUTPUT_STR (lsp, ASE_T(" "));
			if (__print (lsp, ASE_LSP_FBODY(obj), ase_false) == -1) return -1;
			OUTPUT_STR (lsp, ASE_T(")"));
			break;

		case ASE_LSP_OBJ_MACRO:
			OUTPUT_STR (lsp, ASE_T("(macro "));
			if (__print (lsp, ASE_LSP_FFORMAL(obj), ase_true) == -1) return -1;
			OUTPUT_STR (lsp, ASE_T(" "));
			if (__print (lsp, ASE_LSP_FBODY(obj), ase_false) == -1) return -1;
			OUTPUT_STR (lsp, ASE_T(")"));
			break;
		case ASE_LSP_OBJ_PRIM:
			OUTPUT_STR (lsp, ASE_T("prim"));
			break;

		default:
			lsp->prmfns.misc.sprintf (
				lsp->prmfns.misc.custom_data,
				buf, ASE_COUNTOF(buf),
				ASE_T("unknown object type: %d"), ASE_LSP_TYPE(obj)); 
			OUTPUT_STR (lsp, buf);
	}

	return 0;
}

int ase_lsp_print (ase_lsp_t* lsp, const ase_lsp_obj_t* obj)
{
	return __print (lsp, obj, ase_true);
}
