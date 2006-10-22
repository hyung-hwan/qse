/*
 * $Id: prim_compar.c,v 1.3 2006-10-22 13:10:46 bacon Exp $
 */

#include <sse/lsp/prim.h>
#include <sse/bas/assert.h>

sse_lsp_obj_t* sse_lsp_prim_eq (sse_lsp_t* lsp, sse_lsp_obj_t* args)
{
	sse_lsp_obj_t* p1, * p2;
	int res;

	SSE_LSP_PRIM_CHECK_ARG_COUNT (lsp, args, 2, 2);
	sse_assert (SSE_LSP_TYPE(args) == SSE_LSP_OBJ_CONS);

	p1 = sse_lsp_eval (lsp, SSE_LSP_CAR(args));
	if (p1 == SSE_NULL) return SSE_NULL;
	// TODO: lock p1....

	p2 = sse_lsp_eval (lsp, SSE_LSP_CAR(SSE_LSP_CDR(args)));
	if (p2 == SSE_NULL) return SSE_NULL;

	if (SSE_LSP_TYPE(p1) == SSE_LSP_OBJ_INT) {
		if (SSE_LSP_TYPE(p2) == SSE_LSP_OBJ_INT) {
			res = SSE_LSP_IVALUE(p1) == SSE_LSP_IVALUE(p2);
		}
		else if (SSE_LSP_TYPE(p2) == SSE_LSP_OBJ_REAL) {
			res = SSE_LSP_IVALUE(p1) == SSE_LSP_RVALUE(p2);
		}
		else {
			lsp->errnum = SSE_LSP_ERR_BAD_VALUE;
			return SSE_NULL;
		}
	}
	else if (SSE_LSP_TYPE(p1) == SSE_LSP_OBJ_REAL) {
		if (SSE_LSP_TYPE(p2) == SSE_LSP_OBJ_INT) {
			res = SSE_LSP_RVALUE(p1) == SSE_LSP_IVALUE(p2);
		}
		else if (SSE_LSP_TYPE(p2) == SSE_LSP_OBJ_REAL) {
			res = SSE_LSP_RVALUE(p1) == SSE_LSP_RVALUE(p2);
		}
		else {
			lsp->errnum = SSE_LSP_ERR_BAD_VALUE;
			return SSE_NULL;
		}
	}
	else if (SSE_LSP_TYPE(p1) == SSE_LSP_OBJ_SYMBOL) {
		if (SSE_LSP_TYPE(p2) == SSE_LSP_OBJ_SYMBOL) {
			res = sse_lsp_comp_symbol2 (
				p1, SSE_LSP_SYMVALUE(p2), SSE_LSP_SYMLEN(p2)) == 0;
		}
		else {
			lsp->errnum = SSE_LSP_ERR_BAD_VALUE;
			return SSE_NULL;
		}
	}
	else if (SSE_LSP_TYPE(p1) == SSE_LSP_OBJ_STRING) {
		if (SSE_LSP_TYPE(p2) == SSE_LSP_OBJ_STRING) {
			res = sse_lsp_comp_string2 (
				p1, SSE_LSP_STRVALUE(p2), SSE_LSP_STRLEN(p2)) == 0;
		}
		else {
			lsp->errnum = SSE_LSP_ERR_BAD_VALUE;
			return SSE_NULL;
		}
	}
	else {
		lsp->errnum = SSE_LSP_ERR_BAD_VALUE;
		return SSE_NULL;
	}

	return (res)? lsp->mem->t: lsp->mem->nil;
}

sse_lsp_obj_t* sse_lsp_prim_ne (sse_lsp_t* lsp, sse_lsp_obj_t* args)
{
	sse_lsp_obj_t* p1, * p2;
	int res;

	SSE_LSP_PRIM_CHECK_ARG_COUNT (lsp, args, 2, 2);
	sse_assert (SSE_LSP_TYPE(args) == SSE_LSP_OBJ_CONS);

	p1 = sse_lsp_eval (lsp, SSE_LSP_CAR(args));
	if (p1 == SSE_NULL) return SSE_NULL;
	// TODO: lock p1....

	p2 = sse_lsp_eval (lsp, SSE_LSP_CAR(SSE_LSP_CDR(args)));
	if (p2 == SSE_NULL) return SSE_NULL;

	if (SSE_LSP_TYPE(p1) == SSE_LSP_OBJ_INT) {
		if (SSE_LSP_TYPE(p2) == SSE_LSP_OBJ_INT) {
			res = SSE_LSP_IVALUE(p1) != SSE_LSP_IVALUE(p2);
		}
		else if (SSE_LSP_TYPE(p2) == SSE_LSP_OBJ_REAL) {
			res = SSE_LSP_IVALUE(p1) != SSE_LSP_RVALUE(p2);
		}
		else {
			lsp->errnum = SSE_LSP_ERR_BAD_VALUE;
			return SSE_NULL;
		}
	}
	else if (SSE_LSP_TYPE(p1) == SSE_LSP_OBJ_REAL) {
		if (SSE_LSP_TYPE(p2) == SSE_LSP_OBJ_INT) {
			res = SSE_LSP_RVALUE(p1) != SSE_LSP_IVALUE(p2);
		}
		else if (SSE_LSP_TYPE(p2) == SSE_LSP_OBJ_REAL) {
			res = SSE_LSP_RVALUE(p1) != SSE_LSP_RVALUE(p2);
		}
		else {
			lsp->errnum = SSE_LSP_ERR_BAD_VALUE;
			return SSE_NULL;
		}
	}
	else if (SSE_LSP_TYPE(p1) == SSE_LSP_OBJ_SYMBOL) {
		if (SSE_LSP_TYPE(p2) == SSE_LSP_OBJ_SYMBOL) {
			res = sse_lsp_comp_symbol2 (
				p1, SSE_LSP_SYMVALUE(p2), SSE_LSP_SYMLEN(p2)) != 0;
		}
		else {
			lsp->errnum = SSE_LSP_ERR_BAD_VALUE;
			return SSE_NULL;
		}
	}
	else if (SSE_LSP_TYPE(p1) == SSE_LSP_OBJ_STRING) {
		if (SSE_LSP_TYPE(p2) == SSE_LSP_OBJ_STRING) {
			res = sse_lsp_comp_string2 (
				p1, SSE_LSP_STRVALUE(p2), SSE_LSP_STRLEN(p2)) != 0;
		}
		else {
			lsp->errnum = SSE_LSP_ERR_BAD_VALUE;
			return SSE_NULL;
		}
	}
	else {
		lsp->errnum = SSE_LSP_ERR_BAD_VALUE;
		return SSE_NULL;
	}

	return (res)? lsp->mem->t: lsp->mem->nil;
}

sse_lsp_obj_t* sse_lsp_prim_gt (sse_lsp_t* lsp, sse_lsp_obj_t* args)
{
	sse_lsp_obj_t* p1, * p2;
	int res;

	SSE_LSP_PRIM_CHECK_ARG_COUNT (lsp, args, 2, 2);
	sse_assert (SSE_LSP_TYPE(args) == SSE_LSP_OBJ_CONS);

	p1 = sse_lsp_eval (lsp, SSE_LSP_CAR(args));
	if (p1 == SSE_NULL) return SSE_NULL;
	// TODO: lock p1....

	p2 = sse_lsp_eval (lsp, SSE_LSP_CAR(SSE_LSP_CDR(args)));
	if (p2 == SSE_NULL) return SSE_NULL;

	if (SSE_LSP_TYPE(p1) == SSE_LSP_OBJ_INT) {
		if (SSE_LSP_TYPE(p2) == SSE_LSP_OBJ_INT) {
			res = SSE_LSP_IVALUE(p1) > SSE_LSP_IVALUE(p2);
		}
		else if (SSE_LSP_TYPE(p2) == SSE_LSP_OBJ_REAL) {
			res = SSE_LSP_IVALUE(p1) > SSE_LSP_RVALUE(p2);
		}
		else {
			lsp->errnum = SSE_LSP_ERR_BAD_VALUE;
			return SSE_NULL;
		}
	}
	else if (SSE_LSP_TYPE(p1) == SSE_LSP_OBJ_REAL) {
		if (SSE_LSP_TYPE(p2) == SSE_LSP_OBJ_INT) {
			res = SSE_LSP_RVALUE(p1) > SSE_LSP_IVALUE(p2);
		}
		else if (SSE_LSP_TYPE(p2) == SSE_LSP_OBJ_REAL) {
			res = SSE_LSP_RVALUE(p1) > SSE_LSP_RVALUE(p2);
		}
		else {
			lsp->errnum = SSE_LSP_ERR_BAD_VALUE;
			return SSE_NULL;
		}
	}
	else if (SSE_LSP_TYPE(p1) == SSE_LSP_OBJ_SYMBOL) {
		if (SSE_LSP_TYPE(p2) == SSE_LSP_OBJ_SYMBOL) {
			res = sse_lsp_comp_symbol2 (
				p1, SSE_LSP_SYMVALUE(p2), SSE_LSP_SYMLEN(p2)) > 0;
		}
		else {
			lsp->errnum = SSE_LSP_ERR_BAD_VALUE;
			return SSE_NULL;
		}
	}
	else if (SSE_LSP_TYPE(p1) == SSE_LSP_OBJ_STRING) {
		if (SSE_LSP_TYPE(p2) == SSE_LSP_OBJ_STRING) {
			res = sse_lsp_comp_string2 (
				p1, SSE_LSP_STRVALUE(p2), SSE_LSP_STRLEN(p2)) > 0;
		}
		else {
			lsp->errnum = SSE_LSP_ERR_BAD_VALUE;
			return SSE_NULL;
		}
	}
	else {
		lsp->errnum = SSE_LSP_ERR_BAD_VALUE;
		return SSE_NULL;
	}

	return (res)? lsp->mem->t: lsp->mem->nil;
}

sse_lsp_obj_t* sse_lsp_prim_lt (sse_lsp_t* lsp, sse_lsp_obj_t* args)
{
	sse_lsp_obj_t* p1, * p2;
	int res;

	SSE_LSP_PRIM_CHECK_ARG_COUNT (lsp, args, 2, 2);
	sse_assert (SSE_LSP_TYPE(args) == SSE_LSP_OBJ_CONS);

	p1 = sse_lsp_eval (lsp, SSE_LSP_CAR(args));
	if (p1 == SSE_NULL) return SSE_NULL;
	// TODO: lock p1....

	p2 = sse_lsp_eval (lsp, SSE_LSP_CAR(SSE_LSP_CDR(args)));
	if (p2 == SSE_NULL) return SSE_NULL;

	if (SSE_LSP_TYPE(p1) == SSE_LSP_OBJ_INT) {
		if (SSE_LSP_TYPE(p2) == SSE_LSP_OBJ_INT) {
			res = SSE_LSP_IVALUE(p1) < SSE_LSP_IVALUE(p2);
		}
		else if (SSE_LSP_TYPE(p2) == SSE_LSP_OBJ_REAL) {
			res = SSE_LSP_IVALUE(p1) < SSE_LSP_RVALUE(p2);
		}
		else {
			lsp->errnum = SSE_LSP_ERR_BAD_VALUE;
			return SSE_NULL;
		}
	}
	else if (SSE_LSP_TYPE(p1) == SSE_LSP_OBJ_REAL) {
		if (SSE_LSP_TYPE(p2) == SSE_LSP_OBJ_INT) {
			res = SSE_LSP_RVALUE(p1) < SSE_LSP_IVALUE(p2);
		}
		else if (SSE_LSP_TYPE(p2) == SSE_LSP_OBJ_REAL) {
			res = SSE_LSP_RVALUE(p1) < SSE_LSP_RVALUE(p2);
		}
		else {
			lsp->errnum = SSE_LSP_ERR_BAD_VALUE;
			return SSE_NULL;
		}
	}
	else if (SSE_LSP_TYPE(p1) == SSE_LSP_OBJ_SYMBOL) {
		if (SSE_LSP_TYPE(p2) == SSE_LSP_OBJ_SYMBOL) {
			res = sse_lsp_comp_symbol2 (
				p1, SSE_LSP_SYMVALUE(p2), SSE_LSP_SYMLEN(p2)) < 0;
		}
		else {
			lsp->errnum = SSE_LSP_ERR_BAD_VALUE;
			return SSE_NULL;
		}
	}
	else if (SSE_LSP_TYPE(p1) == SSE_LSP_OBJ_STRING) {
		if (SSE_LSP_TYPE(p2) == SSE_LSP_OBJ_STRING) {
			res = sse_lsp_comp_string2 (
				p1, SSE_LSP_STRVALUE(p2), SSE_LSP_STRLEN(p2)) < 0;
		}
		else {
			lsp->errnum = SSE_LSP_ERR_BAD_VALUE;
			return SSE_NULL;
		}
	}
	else {
		lsp->errnum = SSE_LSP_ERR_BAD_VALUE;
		return SSE_NULL;
	}

	return (res)? lsp->mem->t: lsp->mem->nil;
}

sse_lsp_obj_t* sse_lsp_prim_ge (sse_lsp_t* lsp, sse_lsp_obj_t* args)
{
	sse_lsp_obj_t* p1, * p2;
	int res;

	SSE_LSP_PRIM_CHECK_ARG_COUNT (lsp, args, 2, 2);
	sse_assert (SSE_LSP_TYPE(args) == SSE_LSP_OBJ_CONS);

	p1 = sse_lsp_eval (lsp, SSE_LSP_CAR(args));
	if (p1 == SSE_NULL) return SSE_NULL;
	// TODO: lock p1....

	p2 = sse_lsp_eval (lsp, SSE_LSP_CAR(SSE_LSP_CDR(args)));
	if (p2 == SSE_NULL) return SSE_NULL;

	if (SSE_LSP_TYPE(p1) == SSE_LSP_OBJ_INT) {
		if (SSE_LSP_TYPE(p2) == SSE_LSP_OBJ_INT) {
			res = SSE_LSP_IVALUE(p1) >= SSE_LSP_IVALUE(p2);
		}
		else if (SSE_LSP_TYPE(p2) == SSE_LSP_OBJ_REAL) {
			res = SSE_LSP_IVALUE(p1) >= SSE_LSP_RVALUE(p2);
		}
		else {
			lsp->errnum = SSE_LSP_ERR_BAD_VALUE;
			return SSE_NULL;
		}
	}
	else if (SSE_LSP_TYPE(p1) == SSE_LSP_OBJ_REAL) {
		if (SSE_LSP_TYPE(p2) == SSE_LSP_OBJ_INT) {
			res = SSE_LSP_RVALUE(p1) >= SSE_LSP_IVALUE(p2);
		}
		else if (SSE_LSP_TYPE(p2) == SSE_LSP_OBJ_REAL) {
			res = SSE_LSP_RVALUE(p1) >= SSE_LSP_RVALUE(p2);
		}
		else {
			lsp->errnum = SSE_LSP_ERR_BAD_VALUE;
			return SSE_NULL;
		}
	}
	else if (SSE_LSP_TYPE(p1) == SSE_LSP_OBJ_SYMBOL) {
		if (SSE_LSP_TYPE(p2) == SSE_LSP_OBJ_SYMBOL) {
			res = sse_lsp_comp_symbol2 (
				p1, SSE_LSP_SYMVALUE(p2), SSE_LSP_SYMLEN(p2)) >= 0;
		}
		else {
			lsp->errnum = SSE_LSP_ERR_BAD_VALUE;
			return SSE_NULL;
		}
	}
	else if (SSE_LSP_TYPE(p1) == SSE_LSP_OBJ_STRING) {
		if (SSE_LSP_TYPE(p2) == SSE_LSP_OBJ_STRING) {
			res = sse_lsp_comp_string2 (
				p1, SSE_LSP_STRVALUE(p2), SSE_LSP_STRLEN(p2)) >= 0;
		}
		else {
			lsp->errnum = SSE_LSP_ERR_BAD_VALUE;
			return SSE_NULL;
		}
	}
	else {
		lsp->errnum = SSE_LSP_ERR_BAD_VALUE;
		return SSE_NULL;
	}

	return (res)? lsp->mem->t: lsp->mem->nil;
}

sse_lsp_obj_t* sse_lsp_prim_le (sse_lsp_t* lsp, sse_lsp_obj_t* args)
{
	sse_lsp_obj_t* p1, * p2;
	int res;

	SSE_LSP_PRIM_CHECK_ARG_COUNT (lsp, args, 2, 2);
	sse_assert (SSE_LSP_TYPE(args) == SSE_LSP_OBJ_CONS);

	p1 = sse_lsp_eval (lsp, SSE_LSP_CAR(args));
	if (p1 == SSE_NULL) return SSE_NULL;
	// TODO: lock p1....

	p2 = sse_lsp_eval (lsp, SSE_LSP_CAR(SSE_LSP_CDR(args)));
	if (p2 == SSE_NULL) return SSE_NULL;

	if (SSE_LSP_TYPE(p1) == SSE_LSP_OBJ_INT) {
		if (SSE_LSP_TYPE(p2) == SSE_LSP_OBJ_INT) {
			res = SSE_LSP_IVALUE(p1) <= SSE_LSP_IVALUE(p2);
		}
		else if (SSE_LSP_TYPE(p2) == SSE_LSP_OBJ_REAL) {
			res = SSE_LSP_IVALUE(p1) <= SSE_LSP_RVALUE(p2);
		}
		else {
			lsp->errnum = SSE_LSP_ERR_BAD_VALUE;
			return SSE_NULL;
		}
	}
	else if (SSE_LSP_TYPE(p1) == SSE_LSP_OBJ_REAL) {
		if (SSE_LSP_TYPE(p2) == SSE_LSP_OBJ_INT) {
			res = SSE_LSP_RVALUE(p1) <= SSE_LSP_IVALUE(p2);
		}
		else if (SSE_LSP_TYPE(p2) == SSE_LSP_OBJ_REAL) {
			res = SSE_LSP_RVALUE(p1) <= SSE_LSP_RVALUE(p2);
		}
		else {
			lsp->errnum = SSE_LSP_ERR_BAD_VALUE;
			return SSE_NULL;
		}
	}
	else if (SSE_LSP_TYPE(p1) == SSE_LSP_OBJ_SYMBOL) {
		if (SSE_LSP_TYPE(p2) == SSE_LSP_OBJ_SYMBOL) {
			res = sse_lsp_comp_symbol2 (
				p1, SSE_LSP_SYMVALUE(p2), SSE_LSP_SYMLEN(p2)) <= 0;
		}
		else {
			lsp->errnum = SSE_LSP_ERR_BAD_VALUE;
			return SSE_NULL;
		}
	}
	else if (SSE_LSP_TYPE(p1) == SSE_LSP_OBJ_STRING) {
		if (SSE_LSP_TYPE(p2) == SSE_LSP_OBJ_STRING) {
			res = sse_lsp_comp_string2 (
				p1, SSE_LSP_STRVALUE(p2), SSE_LSP_STRLEN(p2)) <= 0;
		}
		else {
			lsp->errnum = SSE_LSP_ERR_BAD_VALUE;
			return SSE_NULL;
		}
	}
	else {
		lsp->errnum = SSE_LSP_ERR_BAD_VALUE;
		return SSE_NULL;
	}

	return (res)? lsp->mem->t: lsp->mem->nil;
}
