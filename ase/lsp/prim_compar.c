/*
 * $Id: prim_compar.c,v 1.4 2006-10-24 04:22:39 bacon Exp $
 */

#include <ase/lsp/prim.h>
#include <ase/bas/assert.h>

ase_lsp_obj_t* ase_lsp_prim_eq (ase_lsp_t* lsp, ase_lsp_obj_t* args)
{
	ase_lsp_obj_t* p1, * p2;
	int res;

	ASE_LSP_PRIM_CHECK_ARG_COUNT (lsp, args, 2, 2);
	ase_assert (ASE_LSP_TYPE(args) == ASE_LSP_OBJ_CONS);

	p1 = ase_lsp_eval (lsp, ASE_LSP_CAR(args));
	if (p1 == ASE_NULL) return ASE_NULL;
	// TODO: lock p1....

	p2 = ase_lsp_eval (lsp, ASE_LSP_CAR(ASE_LSP_CDR(args)));
	if (p2 == ASE_NULL) return ASE_NULL;

	if (ASE_LSP_TYPE(p1) == ASE_LSP_OBJ_INT) {
		if (ASE_LSP_TYPE(p2) == ASE_LSP_OBJ_INT) {
			res = ASE_LSP_IVALUE(p1) == ASE_LSP_IVALUE(p2);
		}
		else if (ASE_LSP_TYPE(p2) == ASE_LSP_OBJ_REAL) {
			res = ASE_LSP_IVALUE(p1) == ASE_LSP_RVALUE(p2);
		}
		else {
			lsp->errnum = ASE_LSP_ERR_BAD_VALUE;
			return ASE_NULL;
		}
	}
	else if (ASE_LSP_TYPE(p1) == ASE_LSP_OBJ_REAL) {
		if (ASE_LSP_TYPE(p2) == ASE_LSP_OBJ_INT) {
			res = ASE_LSP_RVALUE(p1) == ASE_LSP_IVALUE(p2);
		}
		else if (ASE_LSP_TYPE(p2) == ASE_LSP_OBJ_REAL) {
			res = ASE_LSP_RVALUE(p1) == ASE_LSP_RVALUE(p2);
		}
		else {
			lsp->errnum = ASE_LSP_ERR_BAD_VALUE;
			return ASE_NULL;
		}
	}
	else if (ASE_LSP_TYPE(p1) == ASE_LSP_OBJ_SYMBOL) {
		if (ASE_LSP_TYPE(p2) == ASE_LSP_OBJ_SYMBOL) {
			res = ase_lsp_comp_symbol2 (
				p1, ASE_LSP_SYMVALUE(p2), ASE_LSP_SYMLEN(p2)) == 0;
		}
		else {
			lsp->errnum = ASE_LSP_ERR_BAD_VALUE;
			return ASE_NULL;
		}
	}
	else if (ASE_LSP_TYPE(p1) == ASE_LSP_OBJ_STRING) {
		if (ASE_LSP_TYPE(p2) == ASE_LSP_OBJ_STRING) {
			res = ase_lsp_comp_string2 (
				p1, ASE_LSP_STRVALUE(p2), ASE_LSP_STRLEN(p2)) == 0;
		}
		else {
			lsp->errnum = ASE_LSP_ERR_BAD_VALUE;
			return ASE_NULL;
		}
	}
	else {
		lsp->errnum = ASE_LSP_ERR_BAD_VALUE;
		return ASE_NULL;
	}

	return (res)? lsp->mem->t: lsp->mem->nil;
}

ase_lsp_obj_t* ase_lsp_prim_ne (ase_lsp_t* lsp, ase_lsp_obj_t* args)
{
	ase_lsp_obj_t* p1, * p2;
	int res;

	ASE_LSP_PRIM_CHECK_ARG_COUNT (lsp, args, 2, 2);
	ase_assert (ASE_LSP_TYPE(args) == ASE_LSP_OBJ_CONS);

	p1 = ase_lsp_eval (lsp, ASE_LSP_CAR(args));
	if (p1 == ASE_NULL) return ASE_NULL;
	// TODO: lock p1....

	p2 = ase_lsp_eval (lsp, ASE_LSP_CAR(ASE_LSP_CDR(args)));
	if (p2 == ASE_NULL) return ASE_NULL;

	if (ASE_LSP_TYPE(p1) == ASE_LSP_OBJ_INT) {
		if (ASE_LSP_TYPE(p2) == ASE_LSP_OBJ_INT) {
			res = ASE_LSP_IVALUE(p1) != ASE_LSP_IVALUE(p2);
		}
		else if (ASE_LSP_TYPE(p2) == ASE_LSP_OBJ_REAL) {
			res = ASE_LSP_IVALUE(p1) != ASE_LSP_RVALUE(p2);
		}
		else {
			lsp->errnum = ASE_LSP_ERR_BAD_VALUE;
			return ASE_NULL;
		}
	}
	else if (ASE_LSP_TYPE(p1) == ASE_LSP_OBJ_REAL) {
		if (ASE_LSP_TYPE(p2) == ASE_LSP_OBJ_INT) {
			res = ASE_LSP_RVALUE(p1) != ASE_LSP_IVALUE(p2);
		}
		else if (ASE_LSP_TYPE(p2) == ASE_LSP_OBJ_REAL) {
			res = ASE_LSP_RVALUE(p1) != ASE_LSP_RVALUE(p2);
		}
		else {
			lsp->errnum = ASE_LSP_ERR_BAD_VALUE;
			return ASE_NULL;
		}
	}
	else if (ASE_LSP_TYPE(p1) == ASE_LSP_OBJ_SYMBOL) {
		if (ASE_LSP_TYPE(p2) == ASE_LSP_OBJ_SYMBOL) {
			res = ase_lsp_comp_symbol2 (
				p1, ASE_LSP_SYMVALUE(p2), ASE_LSP_SYMLEN(p2)) != 0;
		}
		else {
			lsp->errnum = ASE_LSP_ERR_BAD_VALUE;
			return ASE_NULL;
		}
	}
	else if (ASE_LSP_TYPE(p1) == ASE_LSP_OBJ_STRING) {
		if (ASE_LSP_TYPE(p2) == ASE_LSP_OBJ_STRING) {
			res = ase_lsp_comp_string2 (
				p1, ASE_LSP_STRVALUE(p2), ASE_LSP_STRLEN(p2)) != 0;
		}
		else {
			lsp->errnum = ASE_LSP_ERR_BAD_VALUE;
			return ASE_NULL;
		}
	}
	else {
		lsp->errnum = ASE_LSP_ERR_BAD_VALUE;
		return ASE_NULL;
	}

	return (res)? lsp->mem->t: lsp->mem->nil;
}

ase_lsp_obj_t* ase_lsp_prim_gt (ase_lsp_t* lsp, ase_lsp_obj_t* args)
{
	ase_lsp_obj_t* p1, * p2;
	int res;

	ASE_LSP_PRIM_CHECK_ARG_COUNT (lsp, args, 2, 2);
	ase_assert (ASE_LSP_TYPE(args) == ASE_LSP_OBJ_CONS);

	p1 = ase_lsp_eval (lsp, ASE_LSP_CAR(args));
	if (p1 == ASE_NULL) return ASE_NULL;
	// TODO: lock p1....

	p2 = ase_lsp_eval (lsp, ASE_LSP_CAR(ASE_LSP_CDR(args)));
	if (p2 == ASE_NULL) return ASE_NULL;

	if (ASE_LSP_TYPE(p1) == ASE_LSP_OBJ_INT) {
		if (ASE_LSP_TYPE(p2) == ASE_LSP_OBJ_INT) {
			res = ASE_LSP_IVALUE(p1) > ASE_LSP_IVALUE(p2);
		}
		else if (ASE_LSP_TYPE(p2) == ASE_LSP_OBJ_REAL) {
			res = ASE_LSP_IVALUE(p1) > ASE_LSP_RVALUE(p2);
		}
		else {
			lsp->errnum = ASE_LSP_ERR_BAD_VALUE;
			return ASE_NULL;
		}
	}
	else if (ASE_LSP_TYPE(p1) == ASE_LSP_OBJ_REAL) {
		if (ASE_LSP_TYPE(p2) == ASE_LSP_OBJ_INT) {
			res = ASE_LSP_RVALUE(p1) > ASE_LSP_IVALUE(p2);
		}
		else if (ASE_LSP_TYPE(p2) == ASE_LSP_OBJ_REAL) {
			res = ASE_LSP_RVALUE(p1) > ASE_LSP_RVALUE(p2);
		}
		else {
			lsp->errnum = ASE_LSP_ERR_BAD_VALUE;
			return ASE_NULL;
		}
	}
	else if (ASE_LSP_TYPE(p1) == ASE_LSP_OBJ_SYMBOL) {
		if (ASE_LSP_TYPE(p2) == ASE_LSP_OBJ_SYMBOL) {
			res = ase_lsp_comp_symbol2 (
				p1, ASE_LSP_SYMVALUE(p2), ASE_LSP_SYMLEN(p2)) > 0;
		}
		else {
			lsp->errnum = ASE_LSP_ERR_BAD_VALUE;
			return ASE_NULL;
		}
	}
	else if (ASE_LSP_TYPE(p1) == ASE_LSP_OBJ_STRING) {
		if (ASE_LSP_TYPE(p2) == ASE_LSP_OBJ_STRING) {
			res = ase_lsp_comp_string2 (
				p1, ASE_LSP_STRVALUE(p2), ASE_LSP_STRLEN(p2)) > 0;
		}
		else {
			lsp->errnum = ASE_LSP_ERR_BAD_VALUE;
			return ASE_NULL;
		}
	}
	else {
		lsp->errnum = ASE_LSP_ERR_BAD_VALUE;
		return ASE_NULL;
	}

	return (res)? lsp->mem->t: lsp->mem->nil;
}

ase_lsp_obj_t* ase_lsp_prim_lt (ase_lsp_t* lsp, ase_lsp_obj_t* args)
{
	ase_lsp_obj_t* p1, * p2;
	int res;

	ASE_LSP_PRIM_CHECK_ARG_COUNT (lsp, args, 2, 2);
	ase_assert (ASE_LSP_TYPE(args) == ASE_LSP_OBJ_CONS);

	p1 = ase_lsp_eval (lsp, ASE_LSP_CAR(args));
	if (p1 == ASE_NULL) return ASE_NULL;
	// TODO: lock p1....

	p2 = ase_lsp_eval (lsp, ASE_LSP_CAR(ASE_LSP_CDR(args)));
	if (p2 == ASE_NULL) return ASE_NULL;

	if (ASE_LSP_TYPE(p1) == ASE_LSP_OBJ_INT) {
		if (ASE_LSP_TYPE(p2) == ASE_LSP_OBJ_INT) {
			res = ASE_LSP_IVALUE(p1) < ASE_LSP_IVALUE(p2);
		}
		else if (ASE_LSP_TYPE(p2) == ASE_LSP_OBJ_REAL) {
			res = ASE_LSP_IVALUE(p1) < ASE_LSP_RVALUE(p2);
		}
		else {
			lsp->errnum = ASE_LSP_ERR_BAD_VALUE;
			return ASE_NULL;
		}
	}
	else if (ASE_LSP_TYPE(p1) == ASE_LSP_OBJ_REAL) {
		if (ASE_LSP_TYPE(p2) == ASE_LSP_OBJ_INT) {
			res = ASE_LSP_RVALUE(p1) < ASE_LSP_IVALUE(p2);
		}
		else if (ASE_LSP_TYPE(p2) == ASE_LSP_OBJ_REAL) {
			res = ASE_LSP_RVALUE(p1) < ASE_LSP_RVALUE(p2);
		}
		else {
			lsp->errnum = ASE_LSP_ERR_BAD_VALUE;
			return ASE_NULL;
		}
	}
	else if (ASE_LSP_TYPE(p1) == ASE_LSP_OBJ_SYMBOL) {
		if (ASE_LSP_TYPE(p2) == ASE_LSP_OBJ_SYMBOL) {
			res = ase_lsp_comp_symbol2 (
				p1, ASE_LSP_SYMVALUE(p2), ASE_LSP_SYMLEN(p2)) < 0;
		}
		else {
			lsp->errnum = ASE_LSP_ERR_BAD_VALUE;
			return ASE_NULL;
		}
	}
	else if (ASE_LSP_TYPE(p1) == ASE_LSP_OBJ_STRING) {
		if (ASE_LSP_TYPE(p2) == ASE_LSP_OBJ_STRING) {
			res = ase_lsp_comp_string2 (
				p1, ASE_LSP_STRVALUE(p2), ASE_LSP_STRLEN(p2)) < 0;
		}
		else {
			lsp->errnum = ASE_LSP_ERR_BAD_VALUE;
			return ASE_NULL;
		}
	}
	else {
		lsp->errnum = ASE_LSP_ERR_BAD_VALUE;
		return ASE_NULL;
	}

	return (res)? lsp->mem->t: lsp->mem->nil;
}

ase_lsp_obj_t* ase_lsp_prim_ge (ase_lsp_t* lsp, ase_lsp_obj_t* args)
{
	ase_lsp_obj_t* p1, * p2;
	int res;

	ASE_LSP_PRIM_CHECK_ARG_COUNT (lsp, args, 2, 2);
	ase_assert (ASE_LSP_TYPE(args) == ASE_LSP_OBJ_CONS);

	p1 = ase_lsp_eval (lsp, ASE_LSP_CAR(args));
	if (p1 == ASE_NULL) return ASE_NULL;
	// TODO: lock p1....

	p2 = ase_lsp_eval (lsp, ASE_LSP_CAR(ASE_LSP_CDR(args)));
	if (p2 == ASE_NULL) return ASE_NULL;

	if (ASE_LSP_TYPE(p1) == ASE_LSP_OBJ_INT) {
		if (ASE_LSP_TYPE(p2) == ASE_LSP_OBJ_INT) {
			res = ASE_LSP_IVALUE(p1) >= ASE_LSP_IVALUE(p2);
		}
		else if (ASE_LSP_TYPE(p2) == ASE_LSP_OBJ_REAL) {
			res = ASE_LSP_IVALUE(p1) >= ASE_LSP_RVALUE(p2);
		}
		else {
			lsp->errnum = ASE_LSP_ERR_BAD_VALUE;
			return ASE_NULL;
		}
	}
	else if (ASE_LSP_TYPE(p1) == ASE_LSP_OBJ_REAL) {
		if (ASE_LSP_TYPE(p2) == ASE_LSP_OBJ_INT) {
			res = ASE_LSP_RVALUE(p1) >= ASE_LSP_IVALUE(p2);
		}
		else if (ASE_LSP_TYPE(p2) == ASE_LSP_OBJ_REAL) {
			res = ASE_LSP_RVALUE(p1) >= ASE_LSP_RVALUE(p2);
		}
		else {
			lsp->errnum = ASE_LSP_ERR_BAD_VALUE;
			return ASE_NULL;
		}
	}
	else if (ASE_LSP_TYPE(p1) == ASE_LSP_OBJ_SYMBOL) {
		if (ASE_LSP_TYPE(p2) == ASE_LSP_OBJ_SYMBOL) {
			res = ase_lsp_comp_symbol2 (
				p1, ASE_LSP_SYMVALUE(p2), ASE_LSP_SYMLEN(p2)) >= 0;
		}
		else {
			lsp->errnum = ASE_LSP_ERR_BAD_VALUE;
			return ASE_NULL;
		}
	}
	else if (ASE_LSP_TYPE(p1) == ASE_LSP_OBJ_STRING) {
		if (ASE_LSP_TYPE(p2) == ASE_LSP_OBJ_STRING) {
			res = ase_lsp_comp_string2 (
				p1, ASE_LSP_STRVALUE(p2), ASE_LSP_STRLEN(p2)) >= 0;
		}
		else {
			lsp->errnum = ASE_LSP_ERR_BAD_VALUE;
			return ASE_NULL;
		}
	}
	else {
		lsp->errnum = ASE_LSP_ERR_BAD_VALUE;
		return ASE_NULL;
	}

	return (res)? lsp->mem->t: lsp->mem->nil;
}

ase_lsp_obj_t* ase_lsp_prim_le (ase_lsp_t* lsp, ase_lsp_obj_t* args)
{
	ase_lsp_obj_t* p1, * p2;
	int res;

	ASE_LSP_PRIM_CHECK_ARG_COUNT (lsp, args, 2, 2);
	ase_assert (ASE_LSP_TYPE(args) == ASE_LSP_OBJ_CONS);

	p1 = ase_lsp_eval (lsp, ASE_LSP_CAR(args));
	if (p1 == ASE_NULL) return ASE_NULL;
	// TODO: lock p1....

	p2 = ase_lsp_eval (lsp, ASE_LSP_CAR(ASE_LSP_CDR(args)));
	if (p2 == ASE_NULL) return ASE_NULL;

	if (ASE_LSP_TYPE(p1) == ASE_LSP_OBJ_INT) {
		if (ASE_LSP_TYPE(p2) == ASE_LSP_OBJ_INT) {
			res = ASE_LSP_IVALUE(p1) <= ASE_LSP_IVALUE(p2);
		}
		else if (ASE_LSP_TYPE(p2) == ASE_LSP_OBJ_REAL) {
			res = ASE_LSP_IVALUE(p1) <= ASE_LSP_RVALUE(p2);
		}
		else {
			lsp->errnum = ASE_LSP_ERR_BAD_VALUE;
			return ASE_NULL;
		}
	}
	else if (ASE_LSP_TYPE(p1) == ASE_LSP_OBJ_REAL) {
		if (ASE_LSP_TYPE(p2) == ASE_LSP_OBJ_INT) {
			res = ASE_LSP_RVALUE(p1) <= ASE_LSP_IVALUE(p2);
		}
		else if (ASE_LSP_TYPE(p2) == ASE_LSP_OBJ_REAL) {
			res = ASE_LSP_RVALUE(p1) <= ASE_LSP_RVALUE(p2);
		}
		else {
			lsp->errnum = ASE_LSP_ERR_BAD_VALUE;
			return ASE_NULL;
		}
	}
	else if (ASE_LSP_TYPE(p1) == ASE_LSP_OBJ_SYMBOL) {
		if (ASE_LSP_TYPE(p2) == ASE_LSP_OBJ_SYMBOL) {
			res = ase_lsp_comp_symbol2 (
				p1, ASE_LSP_SYMVALUE(p2), ASE_LSP_SYMLEN(p2)) <= 0;
		}
		else {
			lsp->errnum = ASE_LSP_ERR_BAD_VALUE;
			return ASE_NULL;
		}
	}
	else if (ASE_LSP_TYPE(p1) == ASE_LSP_OBJ_STRING) {
		if (ASE_LSP_TYPE(p2) == ASE_LSP_OBJ_STRING) {
			res = ase_lsp_comp_string2 (
				p1, ASE_LSP_STRVALUE(p2), ASE_LSP_STRLEN(p2)) <= 0;
		}
		else {
			lsp->errnum = ASE_LSP_ERR_BAD_VALUE;
			return ASE_NULL;
		}
	}
	else {
		lsp->errnum = ASE_LSP_ERR_BAD_VALUE;
		return ASE_NULL;
	}

	return (res)? lsp->mem->t: lsp->mem->nil;
}
