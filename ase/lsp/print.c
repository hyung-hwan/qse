/*
 * $Id: print.c,v 1.2 2005-02-04 16:00:37 bacon Exp $
 */

#include <xp/lisp/lisp.h>

void xp_lisp_print_debug (xp_lisp_obj_t* obj)
{
	switch (XP_LISP_TYPE(obj)) {
	case XP_LISP_OBJ_NIL:
		xp_printf ( XP_LISP_TEXT("nil"));
		break;
	case XP_LISP_OBJ_TRUE:
		xp_printf ( XP_LISP_TEXT("t"));
		break;
	case XP_LISP_OBJ_INT:
		xp_printf ( XP_LISP_TEXT("%d"), XP_LISP_IVALUE(obj));
		break;
	case XP_LISP_OBJ_FLOAT:
		xp_printf ( XP_LISP_TEXT("%f"), XP_LISP_FVALUE(obj));
		break;
	case XP_LISP_OBJ_SYMBOL:
		xp_printf ( XP_LISP_TEXT("%s"), XP_LISP_SYMVALUE(obj));
		break;
	case XP_LISP_OBJ_STRING:
		xp_printf ( XP_LISP_TEXT("%s"), XP_LISP_STRVALUE(obj));
		break;
	case XP_LISP_OBJ_CONS:
		{
			xp_lisp_obj_t* p = obj;
			xp_printf ( XP_LISP_TEXT("("));
			do {
				xp_lisp_print_debug (XP_LISP_CAR(p));
				p = XP_LISP_CDR(p);
				if (XP_LISP_TYPE(p) != XP_LISP_OBJ_NIL) {
					xp_printf ( XP_LISP_TEXT(" "));
					if (XP_LISP_TYPE(p) != XP_LISP_OBJ_CONS) {
						xp_printf ( XP_LISP_TEXT(". "));
						xp_lisp_print_debug (p);
					}
				}
			} while (XP_LISP_TYPE(p) != XP_LISP_OBJ_NIL && XP_LISP_TYPE(p) == XP_LISP_OBJ_CONS);
			xp_printf ( XP_LISP_TEXT(")"));
		}
		break;
	case XP_LISP_OBJ_FUNC:
		xp_printf ( XP_LISP_TEXT("func"));
		break;
	case XP_LISP_OBJ_MACRO:
		xp_printf (XP_LISP_TEXT("macro"));
		break;
	case XP_LISP_OBJ_PRIM:
		xp_printf (XP_LISP_TEXT("prim"));
		break;
	default:
		xp_printf (XP_LISP_TEXT("unknown object type: %d"), XP_LISP_TYPE(obj)); 
	}
}

void xp_lisp_print (xp_lisp_t* lsp, xp_lisp_obj_t* obj)
{
	switch (XP_LISP_TYPE(obj)) {
	case XP_LISP_OBJ_NIL:
		xp_fprintf (lsp->outstream, XP_LISP_TEXT("nil"));
		break;
	case XP_LISP_OBJ_TRUE:
		xp_fprintf (lsp->outstream, XP_LISP_TEXT("t"));
		break;
	case XP_LISP_OBJ_INT:
		xp_fprintf (lsp->outstream, XP_LISP_TEXT("%d"), XP_LISP_IVALUE(obj));
		break;
	case XP_LISP_OBJ_FLOAT:
		xp_fprintf (lsp->outstream, XP_LISP_TEXT("%f"), XP_LISP_FVALUE(obj));
		break;
	case XP_LISP_OBJ_SYMBOL:
		xp_fprintf (lsp->outstream, XP_LISP_TEXT("%s"), XP_LISP_SYMVALUE(obj));
		break;
	case XP_LISP_OBJ_STRING:
		xp_fprintf (lsp->outstream, XP_LISP_TEXT("\"%s\""), XP_LISP_STRVALUE(obj));
		break;
	case XP_LISP_OBJ_CONS:
		{
			xp_lisp_obj_t* p = obj;
			xp_fprintf (lsp->outstream, XP_LISP_TEXT("("));
			do {
				xp_lisp_print (lsp, XP_LISP_CAR(p));
				p = XP_LISP_CDR(p);
				if (p != lsp->mem->nil) {
					xp_fprintf (lsp->outstream, XP_LISP_TEXT(" "));
					if (XP_LISP_TYPE(p) != XP_LISP_OBJ_CONS) {
						xp_fprintf (lsp->outstream, XP_LISP_TEXT(". "));
						xp_lisp_print (lsp, p);
					}
				}
			} while (p != lsp->mem->nil && XP_LISP_TYPE(p) == XP_LISP_OBJ_CONS);
			xp_fprintf (lsp->outstream, XP_LISP_TEXT(")"));
		}
		break;
	case XP_LISP_OBJ_FUNC:
		xp_fprintf (lsp->outstream, XP_LISP_TEXT("func"));
		break;
	case XP_LISP_OBJ_MACRO:
		xp_fprintf (lsp->outstream, XP_LISP_TEXT("macro"));
		break;
	case XP_LISP_OBJ_PRIM:
		xp_fprintf (lsp->outstream, XP_LISP_TEXT("prim"));
		break;
	default:
		xp_fprintf (lsp->outstream, 
			XP_LISP_TEXT("unknown object type: %d"), XP_LISP_TYPE(obj)); 
	}
}

