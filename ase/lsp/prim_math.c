/*
 * $Id: prim_math.c,v 1.9 2006-10-25 13:42:31 bacon Exp $
 */

#include <ase/lsp/prim.h>
#include <ase/bas/assert.h>

ase_lsp_obj_t* ase_lsp_prim_plus (ase_lsp_t* lsp, ase_lsp_obj_t* args)
{
	ase_lsp_obj_t* body, * tmp;
	ase_long_t ivalue = 0;
	ase_real_t rvalue = .0;
	ase_bool_t realnum = ase_false;

	ASE_LSP_PRIM_CHECK_ARG_COUNT (lsp, args, 1, ASE_LSP_PRIM_MAX_ARG_COUNT);
	ase_assert (ASE_LSP_TYPE(args) == ASE_LSP_OBJ_CONS);

	body = args;
	//while (body != lsp->mem->nil) {
	while (ASE_LSP_TYPE(body) == ASE_LSP_OBJ_CONS) {
		tmp = ase_lsp_eval (lsp, ASE_LSP_CAR(body));
		if (tmp == ASE_NULL) {
			/*lsp->errnum = ASE_LSP_ERR_BAD_VALUE; */
			return ASE_NULL;
		}

		if (ASE_LSP_TYPE(tmp) == ASE_LSP_OBJ_INT) {
			if (body == args) {
				ase_assert (realnum == ase_false);
				ivalue = ASE_LSP_IVALUE(tmp);
			}
			else {
				if (!realnum) 
					ivalue = ivalue + ASE_LSP_IVALUE(tmp);
				else
					rvalue = rvalue + ASE_LSP_IVALUE(tmp);
			}
		}
		else if (ASE_LSP_TYPE(tmp) == ASE_LSP_OBJ_REAL) {
			if (body == args) {
				ase_assert (realnum == ase_false);
				realnum = ase_true;
				rvalue = ASE_LSP_RVALUE(tmp);
			}
			else {
				if (!realnum) {
					realnum = ase_true;
					rvalue = (ase_real_t)ivalue;
				}
				rvalue = rvalue + ASE_LSP_RVALUE(tmp);
			}
		}
		else {
			lsp->errnum = ASE_LSP_ERR_BAD_VALUE;	
			return ASE_NULL;
		}


		body = ASE_LSP_CDR(body);
	}

	ase_assert (body == lsp->mem->nil);

	tmp = (realnum)?
		ase_lsp_makerealobj (lsp->mem, rvalue):
		ase_lsp_makeintobj (lsp->mem, ivalue);
	if (tmp == ASE_NULL) {
		lsp->errnum = ASE_LSP_ERR_MEMORY;
		return ASE_NULL;
	}

	return tmp;
}

ase_lsp_obj_t* ase_lsp_prim_minus (ase_lsp_t* lsp, ase_lsp_obj_t* args)
{
	ase_lsp_obj_t* body, * tmp;
	ase_long_t ivalue = 0;
	ase_real_t rvalue = .0;
	ase_bool_t realnum = ase_false;

	ASE_LSP_PRIM_CHECK_ARG_COUNT (lsp, args, 1, ASE_LSP_PRIM_MAX_ARG_COUNT);
	ase_assert (ASE_LSP_TYPE(args) == ASE_LSP_OBJ_CONS);

	body = args;
	//while (body != lsp->mem->nil) {
	while (ASE_LSP_TYPE(body) == ASE_LSP_OBJ_CONS) {
		tmp = ase_lsp_eval (lsp, ASE_LSP_CAR(body));
		if (tmp == ASE_NULL) return ASE_NULL;


		if (ASE_LSP_TYPE(tmp) == ASE_LSP_OBJ_INT) {
			if (body == args) {
				ase_assert (realnum == ase_false);
				ivalue = ASE_LSP_IVALUE(tmp);
			}
			else {
				if (!realnum) 
					ivalue = ivalue - ASE_LSP_IVALUE(tmp);
				else
					rvalue = rvalue - ASE_LSP_IVALUE(tmp);
			}
		}
		else if (ASE_LSP_TYPE(tmp) == ASE_LSP_OBJ_REAL) {
			if (body == args) {
				ase_assert (realnum == ase_false);
				realnum = ase_true;
				rvalue = ASE_LSP_RVALUE(tmp);
			}
			else {
				if (!realnum) {
					realnum = ase_true;
					rvalue = (ase_real_t)ivalue;
				}
				rvalue = rvalue - ASE_LSP_RVALUE(tmp);
			}
		}
		else {
			lsp->errnum = ASE_LSP_ERR_BAD_VALUE;	
			return ASE_NULL;
		}


		body = ASE_LSP_CDR(body);
	}

	ase_assert (body == lsp->mem->nil);

	tmp = (realnum)?
		ase_lsp_makerealobj (lsp->mem, rvalue):
		ase_lsp_makeintobj (lsp->mem, ivalue);
	if (tmp == ASE_NULL) {
		lsp->errnum = ASE_LSP_ERR_MEMORY;
		return ASE_NULL;
	}

	return tmp;
}

ase_lsp_obj_t* ase_lsp_prim_multiply (ase_lsp_t* lsp, ase_lsp_obj_t* args)
{
	ase_lsp_obj_t* body, * tmp;
	ase_long_t ivalue = 0;
	ase_real_t rvalue = .0;
	ase_bool_t realnum = ase_false;

	ASE_LSP_PRIM_CHECK_ARG_COUNT (lsp, args, 1, ASE_LSP_PRIM_MAX_ARG_COUNT);
	ase_assert (ASE_LSP_TYPE(args) == ASE_LSP_OBJ_CONS);

	body = args;
	//while (body != lsp->mem->nil) {
	while (ASE_LSP_TYPE(body) == ASE_LSP_OBJ_CONS) {
		tmp = ase_lsp_eval (lsp, ASE_LSP_CAR(body));
		if (tmp == ASE_NULL) return ASE_NULL;


		if (ASE_LSP_TYPE(tmp) == ASE_LSP_OBJ_INT) {
			if (body == args) {
				ase_assert (realnum == ase_false);
				ivalue = ASE_LSP_IVALUE(tmp);
			}
			else {
				if (!realnum) 
					ivalue = ivalue * ASE_LSP_IVALUE(tmp);
				else
					rvalue = rvalue * ASE_LSP_IVALUE(tmp);
			}
		}
		else if (ASE_LSP_TYPE(tmp) == ASE_LSP_OBJ_REAL) {
			if (body == args) {
				ase_assert (realnum == ase_false);
				realnum = ase_true;
				rvalue = ASE_LSP_RVALUE(tmp);
			}
			else {
				if (!realnum) {
					realnum = ase_true;
					rvalue = (ase_real_t)ivalue;
				}
				rvalue = rvalue * ASE_LSP_RVALUE(tmp);
			}
		}
		else {
			lsp->errnum = ASE_LSP_ERR_BAD_VALUE;	
			return ASE_NULL;
		}


		body = ASE_LSP_CDR(body);
	}

	ase_assert (body == lsp->mem->nil);

	tmp = (realnum)?
		ase_lsp_makerealobj (lsp->mem, rvalue):
		ase_lsp_makeintobj (lsp->mem, ivalue);
	if (tmp == ASE_NULL) {
		lsp->errnum = ASE_LSP_ERR_MEMORY;
		return ASE_NULL;
	}

	return tmp;
}

ase_lsp_obj_t* ase_lsp_prim_divide (ase_lsp_t* lsp, ase_lsp_obj_t* args)
{
	ase_lsp_obj_t* body, * tmp;
	ase_long_t ivalue = 0;
	ase_real_t rvalue = .0;
	ase_bool_t realnum = ase_false;

	ASE_LSP_PRIM_CHECK_ARG_COUNT (lsp, args, 1, ASE_LSP_PRIM_MAX_ARG_COUNT);
	ase_assert (ASE_LSP_TYPE(args) == ASE_LSP_OBJ_CONS);

	body = args;
	//while (body != lsp->mem->nil) {
	while (ASE_LSP_TYPE(body) == ASE_LSP_OBJ_CONS) {
		tmp = ase_lsp_eval (lsp, ASE_LSP_CAR(body));
		if (tmp == ASE_NULL) return ASE_NULL;


		if (ASE_LSP_TYPE(tmp) == ASE_LSP_OBJ_INT) {
			if (body == args) {
				ase_assert (realnum == ase_false);
				ivalue = ASE_LSP_IVALUE(tmp);
			}
			else {
				if (!realnum) {
					if (ASE_LSP_IVALUE(tmp) == 0) {
						lsp->errnum = ASE_LSP_ERR_DIVIDE_BY_ZERO;
						return ASE_NULL;
					}
					ivalue = ivalue / ASE_LSP_IVALUE(tmp);
				}
				else
					rvalue = rvalue / ASE_LSP_IVALUE(tmp);
			}
		}
		else if (ASE_LSP_TYPE(tmp) == ASE_LSP_OBJ_REAL) {
			if (body == args) {
				ase_assert (realnum == ase_false);
				realnum = ase_true;
				rvalue = ASE_LSP_RVALUE(tmp);
			}
			else {
				if (!realnum) {
					realnum = ase_true;
					rvalue = (ase_real_t)ivalue;
				}
				rvalue = rvalue / ASE_LSP_RVALUE(tmp);
			}
		}
		else {
			lsp->errnum = ASE_LSP_ERR_BAD_VALUE;	
			return ASE_NULL;
		}


		body = ASE_LSP_CDR(body);
	}

	ase_assert (body == lsp->mem->nil);

	tmp = (realnum)?
		ase_lsp_makerealobj (lsp->mem, rvalue):
		ase_lsp_makeintobj (lsp->mem, ivalue);
	if (tmp == ASE_NULL) {
		lsp->errnum = ASE_LSP_ERR_MEMORY;
		return ASE_NULL;
	}

	return tmp;
}

ase_lsp_obj_t* ase_lsp_prim_modulus (ase_lsp_t* lsp, ase_lsp_obj_t* args)
{
	ase_lsp_obj_t* body, * tmp;
	ase_long_t ivalue = 0;

	ASE_LSP_PRIM_CHECK_ARG_COUNT (lsp, args, 1, ASE_LSP_PRIM_MAX_ARG_COUNT);
	ase_assert (ASE_LSP_TYPE(args) == ASE_LSP_OBJ_CONS);

	body = args;
	//while (body != lsp->mem->nil) {
	while (ASE_LSP_TYPE(body) == ASE_LSP_OBJ_CONS) {
		tmp = ase_lsp_eval (lsp, ASE_LSP_CAR(body));
		if (tmp == ASE_NULL) return ASE_NULL;

		if (ASE_LSP_TYPE(tmp) == ASE_LSP_OBJ_INT) {
			if (body == args) {
				ivalue = ASE_LSP_IVALUE(tmp);
			}
			else {
				if (ASE_LSP_IVALUE(tmp) == 0) {
					lsp->errnum = ASE_LSP_ERR_DIVIDE_BY_ZERO;
					return ASE_NULL;
				}
				ivalue = ivalue % ASE_LSP_IVALUE(tmp);
			}
		}
		else if (ASE_LSP_TYPE(tmp) == ASE_LSP_OBJ_REAL) {
			if (body == args) {
				ivalue = (ase_long_t)ASE_LSP_RVALUE(tmp);
			}
			else {
				ase_long_t tmpi = (ase_long_t)ASE_LSP_RVALUE(tmp);
				if (tmpi == 0) {
					lsp->errnum = ASE_LSP_ERR_DIVIDE_BY_ZERO;
					return ASE_NULL;
				}
				ivalue = ivalue % tmpi;
			}
		}
		else {
			lsp->errnum = ASE_LSP_ERR_BAD_VALUE;	
			return ASE_NULL;
		}


		body = ASE_LSP_CDR(body);
	}

	ase_assert (body == lsp->mem->nil);

	tmp = ase_lsp_makeintobj (lsp->mem, ivalue);
	if (tmp == ASE_NULL) 
	{
		lsp->errnum = ASE_LSP_ENOMEM;
		return ASE_NULL;
	}

	return tmp;
}
