/*
 * $Id: print.c,v 1.1 2005-02-04 15:39:11 bacon Exp $
 */

#include "lsp.h"

void xp_lisp_print_debug (xp_lisp_obj_t* obj)
{
	switch (RBL_TYPE(obj)) {
	case RBL_OBJ_NIL:
		rb_printf ( RBL_TEXT("nil"));
		break;
	case RBL_OBJ_TRUE:
		rb_printf ( RBL_TEXT("t"));
		break;
	case RBL_OBJ_INT:
		rb_printf ( RBL_TEXT("%d"), RBL_IVALUE(obj));
		break;
	case RBL_OBJ_FLOAT:
		rb_printf ( RBL_TEXT("%f"), RBL_FVALUE(obj));
		break;
	case RBL_OBJ_SYMBOL:
		rb_printf ( RBL_TEXT("%s"), RBL_SYMVALUE(obj));
		break;
	case RBL_OBJ_STRING:
		rb_printf ( RBL_TEXT("%s"), RBL_STRVALUE(obj));
		break;
	case RBL_OBJ_CONS:
		{
			xp_lisp_obj_t* p = obj;
			rb_printf ( RBL_TEXT("("));
			do {
				xp_lisp_print_debug (RBL_CAR(p));
				p = RBL_CDR(p);
				if (RBL_TYPE(p) != RBL_OBJ_NIL) {
					rb_printf ( RBL_TEXT(" "));
					if (RBL_TYPE(p) != RBL_OBJ_CONS) {
						rb_printf ( RBL_TEXT(". "));
						xp_lisp_print_debug (p);
					}
				}
			} while (RBL_TYPE(p) != RBL_OBJ_NIL && RBL_TYPE(p) == RBL_OBJ_CONS);
			rb_printf ( RBL_TEXT(")"));
		}
		break;
	case RBL_OBJ_FUNC:
		rb_printf ( RBL_TEXT("func"));
		break;
	case RBL_OBJ_MACRO:
		rb_printf (RBL_TEXT("macro"));
		break;
	case RBL_OBJ_PRIM:
		rb_printf (RBL_TEXT("prim"));
		break;
	default:
		rb_printf (RBL_TEXT("unknown object type: %d"), RBL_TYPE(obj)); 
	}
}

void xp_lisp_print (xp_lisp_t* lsp, xp_lisp_obj_t* obj)
{
	switch (RBL_TYPE(obj)) {
	case RBL_OBJ_NIL:
		rb_fprintf (lsp->outstream, RBL_TEXT("nil"));
		break;
	case RBL_OBJ_TRUE:
		rb_fprintf (lsp->outstream, RBL_TEXT("t"));
		break;
	case RBL_OBJ_INT:
		rb_fprintf (lsp->outstream, RBL_TEXT("%d"), RBL_IVALUE(obj));
		break;
	case RBL_OBJ_FLOAT:
		rb_fprintf (lsp->outstream, RBL_TEXT("%f"), RBL_FVALUE(obj));
		break;
	case RBL_OBJ_SYMBOL:
		rb_fprintf (lsp->outstream, RBL_TEXT("%s"), RBL_SYMVALUE(obj));
		break;
	case RBL_OBJ_STRING:
		rb_fprintf (lsp->outstream, RBL_TEXT("\"%s\""), RBL_STRVALUE(obj));
		break;
	case RBL_OBJ_CONS:
		{
			xp_lisp_obj_t* p = obj;
			rb_fprintf (lsp->outstream, RBL_TEXT("("));
			do {
				xp_lisp_print (lsp, RBL_CAR(p));
				p = RBL_CDR(p);
				if (p != lsp->mem->nil) {
					rb_fprintf (lsp->outstream, RBL_TEXT(" "));
					if (RBL_TYPE(p) != RBL_OBJ_CONS) {
						rb_fprintf (lsp->outstream, RBL_TEXT(". "));
						xp_lisp_print (lsp, p);
					}
				}
			} while (p != lsp->mem->nil && RBL_TYPE(p) == RBL_OBJ_CONS);
			rb_fprintf (lsp->outstream, RBL_TEXT(")"));
		}
		break;
	case RBL_OBJ_FUNC:
		rb_fprintf (lsp->outstream, RBL_TEXT("func"));
		break;
	case RBL_OBJ_MACRO:
		rb_fprintf (lsp->outstream, RBL_TEXT("macro"));
		break;
	case RBL_OBJ_PRIM:
		rb_fprintf (lsp->outstream, RBL_TEXT("prim"));
		break;
	default:
		rb_fprintf (lsp->outstream, 
			RBL_TEXT("unknown object type: %d"), RBL_TYPE(obj)); 
	}
}

