/*
 * $Id: prim_math.c,v 1.1 2005-09-20 08:05:32 bacon Exp $
 */

#include <xp/lsp/prim.h>
#include <xp/bas/assert.h>

xp_lsp_obj_t* xp_lsp_prim_plus (xp_lsp_t* lsp, xp_lsp_obj_t* args)
{
	xp_lsp_obj_t* body, * tmp;
	xp_lsp_int_t value = 0;

	XP_LSP_PRIM_CHECK_ARG_COUNT (lsp, args, 1, XP_LSP_PRIM_MAX_ARG_COUNT);
	xp_assert (XP_LSP_TYPE(args) == XP_LSP_OBJ_CONS);

	body = args;
	//while (body != lsp->mem->nil) {
	while (XP_LSP_TYPE(body) == XP_LSP_OBJ_CONS) {
		tmp = xp_lsp_eval (lsp, XP_LSP_CAR(body));
		if (tmp == XP_NULL) return XP_NULL;

		if (XP_LSP_TYPE(tmp) != XP_LSP_OBJ_INT) {
			lsp->errnum = XP_LSP_ERR_BAD_VALUE;	
			return XP_NULL;
		}

		value = value + XP_LSP_IVALUE(tmp);
		body = XP_LSP_CDR(body);
	}

	tmp = xp_lsp_make_int (lsp->mem, value);
	if (tmp == XP_NULL) {
		lsp->errnum = XP_LSP_ERR_MEM;
		return XP_NULL;
	}

	return tmp;
}

xp_lsp_obj_t* xp_lsp_prim_minus (xp_lsp_t* lsp, xp_lsp_obj_t* args)
{
	xp_lsp_obj_t* body, * tmp;
	xp_lsp_int_t value = 0;

	XP_LSP_PRIM_CHECK_ARG_COUNT (lsp, args, 1, XP_LSP_PRIM_MAX_ARG_COUNT);
	xp_assert (XP_LSP_TYPE(args) == XP_LSP_OBJ_CONS);

	body = args;
	//while (body != lsp->mem->nil) {
	while (XP_LSP_TYPE(body) == XP_LSP_OBJ_CONS) {
		tmp = xp_lsp_eval (lsp, XP_LSP_CAR(body));
		if (tmp == XP_NULL) return XP_NULL;

		if (XP_LSP_TYPE(tmp) != XP_LSP_OBJ_INT) {
			lsp->errnum = XP_LSP_ERR_BAD_VALUE;	
			return XP_NULL;
		}

		value = value - XP_LSP_IVALUE(tmp);
		body = XP_LSP_CDR(body);
	}

	tmp = xp_lsp_make_int (lsp->mem, value);
	if (tmp == XP_NULL) {
		lsp->errnum = XP_LSP_ERR_MEM;
		return XP_NULL;
	}

	return tmp;
}
