/*
 * $Id: print.c,v 1.8 2005-09-18 13:06:43 bacon Exp $
 */

#include <xp/lsp/lsp.h>
#include <xp/bas/stdio.h>
#include <xp/bas/string.h>

void xp_lsp_print_debug (xp_lsp_obj_t* obj)
{
	switch (XP_LSP_TYPE(obj)) {
	case XP_LSP_OBJ_NIL:
		xp_printf (XP_TEXT("nil"));
		break;
	case XP_LSP_OBJ_TRUE:
		xp_printf (XP_TEXT("t"));
		break;
	case XP_LSP_OBJ_INT:
		xp_printf (XP_TEXT("%d"), XP_LSP_IVALUE(obj));
		break;
	case XP_LSP_OBJ_FLOAT:
		xp_printf (XP_TEXT("%f"), XP_LSP_FVALUE(obj));
		break;
	case XP_LSP_OBJ_SYMBOL:
		xp_printf (XP_TEXT("%s"), XP_LSP_SYMVALUE(obj));
		break;
	case XP_LSP_OBJ_STRING:
		xp_printf (XP_TEXT("%s"), XP_LSP_STRVALUE(obj));
		break;
	case XP_LSP_OBJ_CONS:
		{
			xp_lsp_obj_t* p = obj;
			xp_printf (XP_TEXT("("));
			do {
				xp_lsp_print_debug (XP_LSP_CAR(p));
				p = XP_LSP_CDR(p);
				if (XP_LSP_TYPE(p) != XP_LSP_OBJ_NIL) {
					xp_printf (XP_TEXT(" "));
					if (XP_LSP_TYPE(p) != XP_LSP_OBJ_CONS) {
						xp_printf (XP_TEXT(". "));
						xp_lsp_print_debug (p);
					}
				}
			} while (XP_LSP_TYPE(p) != XP_LSP_OBJ_NIL && XP_LSP_TYPE(p) == XP_LSP_OBJ_CONS);
			xp_printf (XP_TEXT(")"));
		}
		break;
	case XP_LSP_OBJ_FUNC:
		xp_printf (XP_TEXT("func"));
		break;
	case XP_LSP_OBJ_MACRO:
		xp_printf (XP_TEXT("macro"));
		break;
	case XP_LSP_OBJ_PRIM:
		xp_printf (XP_TEXT("prim"));
		break;
	default:
		xp_printf (XP_TEXT("unknown object type: %d"), XP_LSP_TYPE(obj)); 
	}
}

#define OUTPUT_STR(lsp,str) \
	do { \
		if (lsp->output_func(XP_LSP_IO_DATA, lsp->output_arg, (void*)str, xp_strlen(str)) == -1) { \
			lsp->errnum = XP_LSP_ERR_OUTPUT; \
			return -1; \
		} \
	} while (0)

int xp_lsp_print (xp_lsp_t* lsp, const xp_lsp_obj_t* obj)
{
	xp_char_t buf[256];

	if (lsp->output_func != XP_NULL) {
		lsp->errnum = XP_LSP_ERR_OUTPUT_NOT_ATTACHED;
		return -1;
	}

	switch (XP_LSP_TYPE(obj)) {
	case XP_LSP_OBJ_NIL:
		OUTPUT_STR (lsp, XP_TEXT("nil"));
		break;
	case XP_LSP_OBJ_TRUE:
		OUTPUT_STR (lsp, XP_TEXT("t"));
		break;
	case XP_LSP_OBJ_INT:
		if (xp_sizeof(xp_lsp_int_t) == xp_sizeof(int)) {
			xp_sprintf (buf, xp_countof(buf), XP_TEXT("%d"), XP_LSP_IVALUE(obj));
		}
		else if (xp_sizeof(xp_lsp_int_t) == xp_sizeof(long)) {
			xp_sprintf (buf, xp_countof(buf), XP_TEXT("%ld"), XP_LSP_IVALUE(obj));
		}
		else if (xp_sizeof(xp_lsp_int_t) == xp_sizeof(long long)) {
			xp_sprintf (buf, xp_countof(buf), XP_TEXT("%lld"), XP_LSP_IVALUE(obj));
		}

		OUTPUT_STR (lsp, buf);
		break;
	case XP_LSP_OBJ_FLOAT:
		xp_sprintf (buf, xp_countof(buf), XP_TEXT("%f"), XP_LSP_FVALUE(obj));
		OUTPUT_STR (lsp, buf);
		break;
	case XP_LSP_OBJ_SYMBOL:
		OUTPUT_STR (lsp, XP_LSP_SYMVALUE(obj));
		break;
	case XP_LSP_OBJ_STRING:
		OUTPUT_STR (lsp, XP_LSP_STRVALUE(obj));
		break;
	case XP_LSP_OBJ_CONS:
		{
			const xp_lsp_obj_t* p = obj;
			OUTPUT_STR (lsp, XP_TEXT("("));
			do {
				xp_lsp_print (lsp, XP_LSP_CAR(p));
				p = XP_LSP_CDR(p);
				if (p != lsp->mem->nil) {
					OUTPUT_STR (lsp, XP_TEXT(" "));
					if (XP_LSP_TYPE(p) != XP_LSP_OBJ_CONS) {
						OUTPUT_STR (lsp, XP_TEXT(". "));
						xp_lsp_print (lsp, p);
					}
				}
			} while (p != lsp->mem->nil && XP_LSP_TYPE(p) == XP_LSP_OBJ_CONS);
			OUTPUT_STR (lsp, XP_TEXT(")"));
		}
		break;
	case XP_LSP_OBJ_FUNC:
		OUTPUT_STR (lsp, XP_TEXT("func"));
		break;
	case XP_LSP_OBJ_MACRO:
		OUTPUT_STR (lsp, XP_TEXT("macro"));
		break;
	case XP_LSP_OBJ_PRIM:
		OUTPUT_STR (lsp, XP_TEXT("prim"));
		break;
	default:
		xp_sprintf (buf, xp_countof(buf),
			XP_TEXT("unknown object type: %d"), XP_LSP_TYPE(obj)); 
		OUTPUT_STR (lsp, buf);
	}

	return 0;
}

