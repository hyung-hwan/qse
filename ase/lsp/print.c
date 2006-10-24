/*
 * $Id: print.c,v 1.14 2006-10-24 04:22:39 bacon Exp $
 */

#include <ase/lsp/lsp.h>
#include <ase/bas/stdio.h>
#include <ase/bas/string.h>

void ase_lsp_print_debug (ase_lsp_obj_t* obj)
{
	switch (ASE_LSP_TYPE(obj)) {
	case ASE_LSP_OBJ_NIL:
		ase_printf (ASE_TEXT("nil"));
		break;
	case ASE_LSP_OBJ_TRUE:
		ase_printf (ASE_TEXT("t"));
		break;
	case ASE_LSP_OBJ_INT:
		ase_printf (ASE_TEXT("%d"), ASE_LSP_IVALUE(obj));
		break;
	case ASE_LSP_OBJ_REAL:
		ase_printf (ASE_TEXT("%f"), ASE_LSP_RVALUE(obj));
		break;
	case ASE_LSP_OBJ_SYMBOL:
		ase_printf (ASE_TEXT("%s"), ASE_LSP_SYMVALUE(obj));
		break;
	case ASE_LSP_OBJ_STRING:
		ase_printf (ASE_TEXT("%s"), ASE_LSP_STRVALUE(obj));
		break;
	case ASE_LSP_OBJ_CONS:
		{
			ase_lsp_obj_t* p = obj;
			ase_printf (ASE_TEXT("("));
			do {
				ase_lsp_print_debug (ASE_LSP_CAR(p));
				p = ASE_LSP_CDR(p);
				if (ASE_LSP_TYPE(p) != ASE_LSP_OBJ_NIL) {
					ase_printf (ASE_TEXT(" "));
					if (ASE_LSP_TYPE(p) != ASE_LSP_OBJ_CONS) {
						ase_printf (ASE_TEXT(". "));
						ase_lsp_print_debug (p);
					}
				}
			} while (ASE_LSP_TYPE(p) != ASE_LSP_OBJ_NIL && ASE_LSP_TYPE(p) == ASE_LSP_OBJ_CONS);
			ase_printf (ASE_TEXT(")"));
		}
		break;
	case ASE_LSP_OBJ_FUNC:
		ase_printf (ASE_TEXT("func"));
		break;
	case ASE_LSP_OBJ_MACRO:
		ase_printf (ASE_TEXT("macro"));
		break;
	case ASE_LSP_OBJ_PRIM:
		ase_printf (ASE_TEXT("prim"));
		break;
	default:
		ase_printf (ASE_TEXT("unknown object type: %d"), ASE_LSP_TYPE(obj)); 
	}
}

#define OUTPUT_STR(lsp,str) \
	do { \
		if (lsp->output_func(ASE_LSP_IO_DATA, lsp->output_arg, (ase_char_t*)str, ase_strlen(str)) == -1) { \
			lsp->errnum = ASE_LSP_ERR_OUTPUT; \
			return -1; \
		} \
	} while (0)

#define OUTPUT_STRX(lsp,str,len) \
	do { \
		if (lsp->output_func(ASE_LSP_IO_DATA, lsp->output_arg, (ase_char_t*)str, len) == -1) { \
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
		OUTPUT_STR (lsp, ASE_TEXT("nil"));
		break;
	case ASE_LSP_OBJ_TRUE:
		OUTPUT_STR (lsp, ASE_TEXT("t"));
		break;
	case ASE_LSP_OBJ_INT:
		if (ase_sizeof(ase_lsp_int_t) == ase_sizeof(int)) {
			ase_sprintf (buf, ase_countof(buf), ASE_TEXT("%d"), ASE_LSP_IVALUE(obj));
		}
		else if (ase_sizeof(ase_lsp_int_t) == ase_sizeof(long)) {
			ase_sprintf (buf, ase_countof(buf), ASE_TEXT("%ld"), ASE_LSP_IVALUE(obj));
		}
		else if (ase_sizeof(ase_lsp_int_t) == ase_sizeof(long long)) {
			ase_sprintf (buf, ase_countof(buf), ASE_TEXT("%lld"), ASE_LSP_IVALUE(obj));
		}

		OUTPUT_STR (lsp, buf);
		break;
	case ASE_LSP_OBJ_REAL:
		if (ase_sizeof(ase_lsp_real_t) == ase_sizeof(double)) {
			ase_sprintf (buf, ase_countof(buf), ASE_TEXT("%f"), 
				(double)ASE_LSP_RVALUE(obj));
		}
		else if (ase_sizeof(ase_lsp_real_t) == ase_sizeof(long double)) {
			ase_sprintf (buf, ase_countof(buf), ASE_TEXT("%Lf"), 
				(long double)ASE_LSP_RVALUE(obj));
		}

		OUTPUT_STR (lsp, buf);
		break;
	case ASE_LSP_OBJ_SYMBOL:
		OUTPUT_STR (lsp, ASE_LSP_SYMVALUE(obj));
		break;
	case ASE_LSP_OBJ_STRING:
		OUTPUT_STR (lsp, ASE_LSP_STRVALUE(obj));
		break;
	case ASE_LSP_OBJ_CONS:
		{
			const ase_lsp_obj_t* p = obj;
			if (prt_cons_par) OUTPUT_STR (lsp, ASE_TEXT("("));
			do {
				ase_lsp_print (lsp, ASE_LSP_CAR(p));
				p = ASE_LSP_CDR(p);
				if (p != lsp->mem->nil) {
					OUTPUT_STR (lsp, ASE_TEXT(" "));
					if (ASE_LSP_TYPE(p) != ASE_LSP_OBJ_CONS) {
						OUTPUT_STR (lsp, ASE_TEXT(". "));
						ase_lsp_print (lsp, p);
					}
				}
			} while (p != lsp->mem->nil && ASE_LSP_TYPE(p) == ASE_LSP_OBJ_CONS);
			if (prt_cons_par) OUTPUT_STR (lsp, ASE_TEXT(")"));
		}
		break;
	case ASE_LSP_OBJ_FUNC:
		/*OUTPUT_STR (lsp, ASE_TEXT("func"));*/
		OUTPUT_STR (lsp, ASE_TEXT("(lambda "));
		if (__print (lsp, ASE_LSP_FFORMAL(obj), ase_true) == -1) return -1;
		OUTPUT_STR (lsp, ASE_TEXT(" "));
		if (__print (lsp, ASE_LSP_FBODY(obj), ase_false) == -1) return -1;
		OUTPUT_STR (lsp, ASE_TEXT(")"));
		break;
	case ASE_LSP_OBJ_MACRO:
		/*OUTPUT_STR (lsp, ASE_TEXT("macro"));*/
		OUTPUT_STR (lsp, ASE_TEXT("(macro "));
		if (__print (lsp, ASE_LSP_FFORMAL(obj), ase_true) == -1) return -1;
		OUTPUT_STR (lsp, ASE_TEXT(" "));
		if (__print (lsp, ASE_LSP_FBODY(obj), ase_false) == -1) return -1;
		OUTPUT_STR (lsp, ASE_TEXT(")"));
		break;
	case ASE_LSP_OBJ_PRIM:
		OUTPUT_STR (lsp, ASE_TEXT("prim"));
		break;
	default:
		ase_sprintf (buf, ase_countof(buf),
			ASE_TEXT("unknown object type: %d"), ASE_LSP_TYPE(obj)); 
		OUTPUT_STR (lsp, buf);
	}

	return 0;
}

int ase_lsp_print (ase_lsp_t* lsp, const ase_lsp_obj_t* obj)
{
	return __print (lsp, obj, ase_true);
}
