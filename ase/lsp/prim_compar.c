/*
 * $Id: prim_compar.c,v 1.1 2005-09-20 12:06:51 bacon Exp $
 */

#include <xp/lsp/prim.h>
#include <xp/bas/assert.h>

xp_lsp_obj_t* xp_lsp_prim_eq (xp_lsp_t* lsp, xp_lsp_obj_t* args)
{
	xp_lsp_obj_t* p1, * p2;
	int res;

	XP_LSP_PRIM_CHECK_ARG_COUNT (lsp, args, 2, 2);
	xp_assert (XP_LSP_TYPE(args) == XP_LSP_OBJ_CONS);

	p1 = xp_lsp_eval (lsp, XP_LSP_CAR(args));
	if (p1 == XP_NULL) return XP_NULL;
	// TODO: lock p1....

	p2 = xp_lsp_eval (lsp, XP_LSP_CAR(XP_LSP_CDR(args)));
	if (p2 == XP_NULL) return XP_NULL;

	if (XP_LSP_TYPE(p1) == XP_LSP_OBJ_INT) {
		if (XP_LSP_TYPE(p2) == XP_LSP_OBJ_INT) {
			res = XP_LSP_IVALUE(p1) == XP_LSP_IVALUE(p2);
		}
		else if (XP_LSP_TYPE(p2) == XP_LSP_OBJ_REAL) {
			res = XP_LSP_IVALUE(p1) == XP_LSP_RVALUE(p2);
		}
		else {
			lsp->errnum = XP_LSP_ERR_BAD_VALUE;
			return XP_NULL;
		}
	}
	else if (XP_LSP_TYPE(p1) == XP_LSP_OBJ_REAL) {
		if (XP_LSP_TYPE(p2) == XP_LSP_OBJ_INT) {
			res = XP_LSP_RVALUE(p1) == XP_LSP_IVALUE(p2);
		}
		else if (XP_LSP_TYPE(p2) == XP_LSP_OBJ_REAL) {
			res = XP_LSP_RVALUE(p1) == XP_LSP_RVALUE(p2);
		}
		else {
			lsp->errnum = XP_LSP_ERR_BAD_VALUE;
			return XP_NULL;
		}
	}
	else if (XP_LSP_TYPE(p1) == XP_LSP_OBJ_SYMBOL) {
		if (XP_LSP_TYPE(p2) == XP_LSP_OBJ_SYMBOL) {
			res = xp_lsp_comp_symbol2 (
				p1, XP_LSP_SYMVALUE(p2), XP_LSP_SYMLEN(p2)) == 0;
		}
		else {
			lsp->errnum = XP_LSP_ERR_BAD_VALUE;
			return XP_NULL;
		}
	}
	else if (XP_LSP_TYPE(p1) == XP_LSP_OBJ_STRING) {
		if (XP_LSP_TYPE(p2) == XP_LSP_OBJ_STRING) {
			res = xp_lsp_comp_string2 (
				p1, XP_LSP_STRVALUE(p2), XP_LSP_STRLEN(p2)) == 0;
		}
		else {
			lsp->errnum = XP_LSP_ERR_BAD_VALUE;
			return XP_NULL;
		}
	}
	else {
		lsp->errnum = XP_LSP_ERR_BAD_VALUE;
		return XP_NULL;
	}

	return (res)? lsp->mem->t: lsp->mem->nil;
}

xp_lsp_obj_t* xp_lsp_prim_gt (xp_lsp_t* lsp, xp_lsp_obj_t* args)
{
	xp_lsp_obj_t* p1, * p2;
	int res;

	XP_LSP_PRIM_CHECK_ARG_COUNT (lsp, args, 2, 2);
	xp_assert (XP_LSP_TYPE(args) == XP_LSP_OBJ_CONS);

	p1 = xp_lsp_eval (lsp, XP_LSP_CAR(args));
	if (p1 == XP_NULL) return XP_NULL;
	// TODO: lock p1....

	p2 = xp_lsp_eval (lsp, XP_LSP_CAR(XP_LSP_CDR(args)));
	if (p2 == XP_NULL) return XP_NULL;

	if (XP_LSP_TYPE(p1) == XP_LSP_OBJ_INT) {
		if (XP_LSP_TYPE(p2) == XP_LSP_OBJ_INT) {
			res = XP_LSP_IVALUE(p1) > XP_LSP_IVALUE(p2);
		}
		else if (XP_LSP_TYPE(p2) == XP_LSP_OBJ_REAL) {
			res = XP_LSP_IVALUE(p1) > XP_LSP_RVALUE(p2);
		}
		else {
			lsp->errnum = XP_LSP_ERR_BAD_VALUE;
			return XP_NULL;
		}
	}
	else if (XP_LSP_TYPE(p1) == XP_LSP_OBJ_REAL) {
		if (XP_LSP_TYPE(p2) == XP_LSP_OBJ_INT) {
			res = XP_LSP_RVALUE(p1) > XP_LSP_IVALUE(p2);
		}
		else if (XP_LSP_TYPE(p2) == XP_LSP_OBJ_REAL) {
			res = XP_LSP_RVALUE(p1) > XP_LSP_RVALUE(p2);
		}
		else {
			lsp->errnum = XP_LSP_ERR_BAD_VALUE;
			return XP_NULL;
		}
	}
	else if (XP_LSP_TYPE(p1) == XP_LSP_OBJ_SYMBOL) {
		if (XP_LSP_TYPE(p2) == XP_LSP_OBJ_SYMBOL) {
			res = xp_lsp_comp_symbol2 (
				p1, XP_LSP_SYMVALUE(p2), XP_LSP_SYMLEN(p2)) > 0;
		}
		else {
			lsp->errnum = XP_LSP_ERR_BAD_VALUE;
			return XP_NULL;
		}
	}
	else if (XP_LSP_TYPE(p1) == XP_LSP_OBJ_STRING) {
		if (XP_LSP_TYPE(p2) == XP_LSP_OBJ_STRING) {
			res = xp_lsp_comp_string2 (
				p1, XP_LSP_STRVALUE(p2), XP_LSP_STRLEN(p2)) > 0;
		}
		else {
			lsp->errnum = XP_LSP_ERR_BAD_VALUE;
			return XP_NULL;
		}
	}
	else {
		lsp->errnum = XP_LSP_ERR_BAD_VALUE;
		return XP_NULL;
	}

	return (res)? lsp->mem->t: lsp->mem->nil;
}

xp_lsp_obj_t* xp_lsp_prim_lt (xp_lsp_t* lsp, xp_lsp_obj_t* args)
{
	xp_lsp_obj_t* p1, * p2;
	int res;

	XP_LSP_PRIM_CHECK_ARG_COUNT (lsp, args, 2, 2);
	xp_assert (XP_LSP_TYPE(args) == XP_LSP_OBJ_CONS);

	p1 = xp_lsp_eval (lsp, XP_LSP_CAR(args));
	if (p1 == XP_NULL) return XP_NULL;
	// TODO: lock p1....

	p2 = xp_lsp_eval (lsp, XP_LSP_CAR(XP_LSP_CDR(args)));
	if (p2 == XP_NULL) return XP_NULL;

	if (XP_LSP_TYPE(p1) == XP_LSP_OBJ_INT) {
		if (XP_LSP_TYPE(p2) == XP_LSP_OBJ_INT) {
			res = XP_LSP_IVALUE(p1) < XP_LSP_IVALUE(p2);
		}
		else if (XP_LSP_TYPE(p2) == XP_LSP_OBJ_REAL) {
			res = XP_LSP_IVALUE(p1) < XP_LSP_RVALUE(p2);
		}
		else {
			lsp->errnum = XP_LSP_ERR_BAD_VALUE;
			return XP_NULL;
		}
	}
	else if (XP_LSP_TYPE(p1) == XP_LSP_OBJ_REAL) {
		if (XP_LSP_TYPE(p2) == XP_LSP_OBJ_INT) {
			res = XP_LSP_RVALUE(p1) < XP_LSP_IVALUE(p2);
		}
		else if (XP_LSP_TYPE(p2) == XP_LSP_OBJ_REAL) {
			res = XP_LSP_RVALUE(p1) < XP_LSP_RVALUE(p2);
		}
		else {
			lsp->errnum = XP_LSP_ERR_BAD_VALUE;
			return XP_NULL;
		}
	}
	else if (XP_LSP_TYPE(p1) == XP_LSP_OBJ_SYMBOL) {
		if (XP_LSP_TYPE(p2) == XP_LSP_OBJ_SYMBOL) {
			res = xp_lsp_comp_symbol2 (
				p1, XP_LSP_SYMVALUE(p2), XP_LSP_SYMLEN(p2)) < 0;
		}
		else {
			lsp->errnum = XP_LSP_ERR_BAD_VALUE;
			return XP_NULL;
		}
	}
	else if (XP_LSP_TYPE(p1) == XP_LSP_OBJ_STRING) {
		if (XP_LSP_TYPE(p2) == XP_LSP_OBJ_STRING) {
			res = xp_lsp_comp_string2 (
				p1, XP_LSP_STRVALUE(p2), XP_LSP_STRLEN(p2)) < 0;
		}
		else {
			lsp->errnum = XP_LSP_ERR_BAD_VALUE;
			return XP_NULL;
		}
	}
	else {
		lsp->errnum = XP_LSP_ERR_BAD_VALUE;
		return XP_NULL;
	}

	return (res)? lsp->mem->t: lsp->mem->nil;
}
