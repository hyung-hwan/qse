/*
 * $Id: print.c,v 1.16 2006-10-26 08:17:38 bacon Exp $
 */

#include <ase/lsp/lsp_i.h>

#if 0
void ase_lsp_print_debug (ase_lsp_obj_t* obj)
{
	switch (ASE_LSP_TYPE(obj)) {
	case ASE_LSP_OBJ_NIL:
		ase_printf (ASE_T("nil"));
		break;
	case ASE_LSP_OBJ_TRUE:
		ase_printf (ASE_T("t"));
		break;
	case ASE_LSP_OBJ_INT:
		ase_printf (ASE_T("%d"), ASE_LSP_IVALUE(obj));
		break;
	case ASE_LSP_OBJ_REAL:
		ase_printf (ASE_T("%f"), ASE_LSP_RVALUE(obj));
		break;
	case ASE_LSP_OBJ_SYM:
		ase_printf (ASE_T("%s"), ASE_LSP_SYMPTR(obj));
		break;
	case ASE_LSP_OBJ_STR:
		ase_printf (ASE_T("%s"), ASE_LSP_STRPTR(obj));
		break;
	case ASE_LSP_OBJ_CONS:
		{
			ase_lsp_obj_t* p = obj;
			ase_printf (ASE_T("("));
			do {
				ase_lsp_print_debug (ASE_LSP_CAR(p));
				p = ASE_LSP_CDR(p);
				if (ASE_LSP_TYPE(p) != ASE_LSP_OBJ_NIL) {
					ase_printf (ASE_T(" "));
					if (ASE_LSP_TYPE(p) != ASE_LSP_OBJ_CONS) {
						ase_printf (ASE_T(". "));
						ase_lsp_print_debug (p);
					}
				}
			} while (ASE_LSP_TYPE(p) != ASE_LSP_OBJ_NIL && ASE_LSP_TYPE(p) == ASE_LSP_OBJ_CONS);
			ase_printf (ASE_T(")"));
		}
		break;
	case ASE_LSP_OBJ_FUNC:
		ase_printf (ASE_T("func"));
		break;
	case ASE_LSP_OBJ_MACRO:
		ase_printf (ASE_T("macro"));
		break;
	case ASE_LSP_OBJ_PRIM:
		ase_printf (ASE_T("prim"));
		break;
	default:
		ase_printf (ASE_T("unknown object type: %d"), ASE_LSP_TYPE(obj)); 
	}
}
#endif

#define OUTPUT_STR(lsp,str) \
	do { \
		if (lsp->output_func(ASE_LSP_IO_WRITE, lsp->output_arg, (ase_char_t*)str, ase_lsp_strlen(str)) == -1) { \
			lsp->errnum = ASE_LSP_ERR_OUTPUT; \
			return -1; \
		} \
	} while (0)

#define OUTPUT_STRX(lsp,str,len) \
	do { \
		if (lsp->output_func(ASE_LSP_IO_WRITE, lsp->output_arg, (ase_char_t*)str, len) == -1) { \
			lsp->errnum = ASE_LSP_ERR_OUTPUT; \
			return -1; \
		} \
	} while (0)

static int __print (ase_lsp_t* lsp, const ase_lsp_obj_t* obj, ase_bool_t prt_cons_par)
{
	ase_char_t buf[256];

	if (lsp->output_func == ASE_NULL) {
		lsp->errnum = ASE_LSP_ERR_OUTPUT_NOT_ATTACHED;
		return -1;
	}

	switch (ASE_LSP_TYPE(obj)) {
	case ASE_LSP_OBJ_NIL:
		OUTPUT_STR (lsp, ASE_T("nil"));
		break;
	case ASE_LSP_OBJ_TRUE:
		OUTPUT_STR (lsp, ASE_T("t"));
		break;
	case ASE_LSP_OBJ_INT:
		if (ase_sizeof(ase_long_t) == ase_sizeof(int)) {
			lsp->syscas.sprintf (
				buf, ase_countof(buf), 
				ASE_T("%d"), ASE_LSP_IVALUE(obj));
		}
		else if (ase_sizeof(ase_long_t) == ase_sizeof(long)) 
		{
			lsp->syscas.sprintf (
				buf, ase_countof(buf), 
				ASE_T("%ld"), ASE_LSP_IVALUE(obj));
		}
#if defined(__BORLANDC__) || defined(_MSC_VER)
		else if (ase_sizeof(ase_long_t) == ase_sizeof(__int64)) 
		{
			lsp->syscas.sprintf (
				buf, ase_countof(buf), 
				ASE_T("%I64d"), ASE_LSP_IVALUE(obj));
		}
#else
		else if (ase_sizeof(ase_long_t) == ase_sizeof(long long)) 
		{
			lsp->syscas.sprintf (
				buf, ase_countof(buf), 
				ASE_T("%lld"), ASE_LSP_IVALUE(obj));
		}
#endif

		OUTPUT_STR (lsp, buf);
		break;
	case ASE_LSP_OBJ_REAL:
		if (ase_sizeof(ase_real_t) == ase_sizeof(double)) {
			lsp->syscas.sprintf (buf, ase_countof(buf), ASE_T("%f"), 
				(double)ASE_LSP_RVALUE(obj));
		}
		else if (ase_sizeof(ase_real_t) == ase_sizeof(long double)) {
			lsp->syscas.sprintf (buf, ase_countof(buf), ASE_T("%Lf"), 
				(long double)ASE_LSP_RVALUE(obj));
		}

		OUTPUT_STR (lsp, buf);
		break;
	case ASE_LSP_OBJ_SYM:
		OUTPUT_STR (lsp, ASE_LSP_SYMPTR(obj));
		break;
	case ASE_LSP_OBJ_STR:
		OUTPUT_STR (lsp, ASE_LSP_STRPTR(obj));
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
		}
		break;
	case ASE_LSP_OBJ_FUNC:
		/*OUTPUT_STR (lsp, ASE_T("func"));*/
		OUTPUT_STR (lsp, ASE_T("(lambda "));
		if (__print (lsp, ASE_LSP_FFORMAL(obj), ase_true) == -1) return -1;
		OUTPUT_STR (lsp, ASE_T(" "));
		if (__print (lsp, ASE_LSP_FBODY(obj), ase_false) == -1) return -1;
		OUTPUT_STR (lsp, ASE_T(")"));
		break;
	case ASE_LSP_OBJ_MACRO:
		/*OUTPUT_STR (lsp, ASE_T("macro"));*/
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
		lsp->syscas.sprintf (buf, ase_countof(buf),
			ASE_T("unknown object type: %d"), ASE_LSP_TYPE(obj)); 
		OUTPUT_STR (lsp, buf);
	}

	return 0;
}

int ase_lsp_print (ase_lsp_t* lsp, const ase_lsp_obj_t* obj)
{
	return __print (lsp, obj, ase_true);
}
