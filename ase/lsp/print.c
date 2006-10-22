/*
 * $Id: print.c,v 1.13 2006-10-22 13:10:46 bacon Exp $
 */

#include <sse/lsp/lsp.h>
#include <sse/bas/stdio.h>
#include <sse/bas/string.h>

void sse_lsp_print_debug (sse_lsp_obj_t* obj)
{
	switch (SSE_LSP_TYPE(obj)) {
	case SSE_LSP_OBJ_NIL:
		sse_printf (SSE_TEXT("nil"));
		break;
	case SSE_LSP_OBJ_TRUE:
		sse_printf (SSE_TEXT("t"));
		break;
	case SSE_LSP_OBJ_INT:
		sse_printf (SSE_TEXT("%d"), SSE_LSP_IVALUE(obj));
		break;
	case SSE_LSP_OBJ_REAL:
		sse_printf (SSE_TEXT("%f"), SSE_LSP_RVALUE(obj));
		break;
	case SSE_LSP_OBJ_SYMBOL:
		sse_printf (SSE_TEXT("%s"), SSE_LSP_SYMVALUE(obj));
		break;
	case SSE_LSP_OBJ_STRING:
		sse_printf (SSE_TEXT("%s"), SSE_LSP_STRVALUE(obj));
		break;
	case SSE_LSP_OBJ_CONS:
		{
			sse_lsp_obj_t* p = obj;
			sse_printf (SSE_TEXT("("));
			do {
				sse_lsp_print_debug (SSE_LSP_CAR(p));
				p = SSE_LSP_CDR(p);
				if (SSE_LSP_TYPE(p) != SSE_LSP_OBJ_NIL) {
					sse_printf (SSE_TEXT(" "));
					if (SSE_LSP_TYPE(p) != SSE_LSP_OBJ_CONS) {
						sse_printf (SSE_TEXT(". "));
						sse_lsp_print_debug (p);
					}
				}
			} while (SSE_LSP_TYPE(p) != SSE_LSP_OBJ_NIL && SSE_LSP_TYPE(p) == SSE_LSP_OBJ_CONS);
			sse_printf (SSE_TEXT(")"));
		}
		break;
	case SSE_LSP_OBJ_FUNC:
		sse_printf (SSE_TEXT("func"));
		break;
	case SSE_LSP_OBJ_MACRO:
		sse_printf (SSE_TEXT("macro"));
		break;
	case SSE_LSP_OBJ_PRIM:
		sse_printf (SSE_TEXT("prim"));
		break;
	default:
		sse_printf (SSE_TEXT("unknown object type: %d"), SSE_LSP_TYPE(obj)); 
	}
}

#define OUTPUT_STR(lsp,str) \
	do { \
		if (lsp->output_func(SSE_LSP_IO_DATA, lsp->output_arg, (sse_char_t*)str, sse_strlen(str)) == -1) { \
			lsp->errnum = SSE_LSP_ERR_OUTPUT; \
			return -1; \
		} \
	} while (0)

#define OUTPUT_STRX(lsp,str,len) \
	do { \
		if (lsp->output_func(SSE_LSP_IO_DATA, lsp->output_arg, (sse_char_t*)str, len) == -1) { \
			lsp->errnum = SSE_LSP_ERR_OUTPUT; \
			return -1; \
		} \
	} while (0)

static int __print (sse_lsp_t* lsp, const sse_lsp_obj_t* obj, sse_bool_t prt_cons_par)
{
	sse_char_t buf[256];

	if (lsp->output_func == SSE_NULL) {
		lsp->errnum = SSE_LSP_ERR_OUTPUT_NOT_ATTACHED;
		return -1;
	}

	switch (SSE_LSP_TYPE(obj)) {
	case SSE_LSP_OBJ_NIL:
		OUTPUT_STR (lsp, SSE_TEXT("nil"));
		break;
	case SSE_LSP_OBJ_TRUE:
		OUTPUT_STR (lsp, SSE_TEXT("t"));
		break;
	case SSE_LSP_OBJ_INT:
		if (sse_sizeof(sse_lsp_int_t) == sse_sizeof(int)) {
			sse_sprintf (buf, sse_countof(buf), SSE_TEXT("%d"), SSE_LSP_IVALUE(obj));
		}
		else if (sse_sizeof(sse_lsp_int_t) == sse_sizeof(long)) {
			sse_sprintf (buf, sse_countof(buf), SSE_TEXT("%ld"), SSE_LSP_IVALUE(obj));
		}
		else if (sse_sizeof(sse_lsp_int_t) == sse_sizeof(long long)) {
			sse_sprintf (buf, sse_countof(buf), SSE_TEXT("%lld"), SSE_LSP_IVALUE(obj));
		}

		OUTPUT_STR (lsp, buf);
		break;
	case SSE_LSP_OBJ_REAL:
		if (sse_sizeof(sse_lsp_real_t) == sse_sizeof(double)) {
			sse_sprintf (buf, sse_countof(buf), SSE_TEXT("%f"), 
				(double)SSE_LSP_RVALUE(obj));
		}
		else if (sse_sizeof(sse_lsp_real_t) == sse_sizeof(long double)) {
			sse_sprintf (buf, sse_countof(buf), SSE_TEXT("%Lf"), 
				(long double)SSE_LSP_RVALUE(obj));
		}

		OUTPUT_STR (lsp, buf);
		break;
	case SSE_LSP_OBJ_SYMBOL:
		OUTPUT_STR (lsp, SSE_LSP_SYMVALUE(obj));
		break;
	case SSE_LSP_OBJ_STRING:
		OUTPUT_STR (lsp, SSE_LSP_STRVALUE(obj));
		break;
	case SSE_LSP_OBJ_CONS:
		{
			const sse_lsp_obj_t* p = obj;
			if (prt_cons_par) OUTPUT_STR (lsp, SSE_TEXT("("));
			do {
				sse_lsp_print (lsp, SSE_LSP_CAR(p));
				p = SSE_LSP_CDR(p);
				if (p != lsp->mem->nil) {
					OUTPUT_STR (lsp, SSE_TEXT(" "));
					if (SSE_LSP_TYPE(p) != SSE_LSP_OBJ_CONS) {
						OUTPUT_STR (lsp, SSE_TEXT(". "));
						sse_lsp_print (lsp, p);
					}
				}
			} while (p != lsp->mem->nil && SSE_LSP_TYPE(p) == SSE_LSP_OBJ_CONS);
			if (prt_cons_par) OUTPUT_STR (lsp, SSE_TEXT(")"));
		}
		break;
	case SSE_LSP_OBJ_FUNC:
		/*OUTPUT_STR (lsp, SSE_TEXT("func"));*/
		OUTPUT_STR (lsp, SSE_TEXT("(lambda "));
		if (__print (lsp, SSE_LSP_FFORMAL(obj), sse_true) == -1) return -1;
		OUTPUT_STR (lsp, SSE_TEXT(" "));
		if (__print (lsp, SSE_LSP_FBODY(obj), sse_false) == -1) return -1;
		OUTPUT_STR (lsp, SSE_TEXT(")"));
		break;
	case SSE_LSP_OBJ_MACRO:
		/*OUTPUT_STR (lsp, SSE_TEXT("macro"));*/
		OUTPUT_STR (lsp, SSE_TEXT("(macro "));
		if (__print (lsp, SSE_LSP_FFORMAL(obj), sse_true) == -1) return -1;
		OUTPUT_STR (lsp, SSE_TEXT(" "));
		if (__print (lsp, SSE_LSP_FBODY(obj), sse_false) == -1) return -1;
		OUTPUT_STR (lsp, SSE_TEXT(")"));
		break;
	case SSE_LSP_OBJ_PRIM:
		OUTPUT_STR (lsp, SSE_TEXT("prim"));
		break;
	default:
		sse_sprintf (buf, sse_countof(buf),
			SSE_TEXT("unknown object type: %d"), SSE_LSP_TYPE(obj)); 
		OUTPUT_STR (lsp, buf);
	}

	return 0;
}

int sse_lsp_print (sse_lsp_t* lsp, const sse_lsp_obj_t* obj)
{
	return __print (lsp, obj, sse_true);
}
