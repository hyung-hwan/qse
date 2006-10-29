/*
 * $Id: prim_math.c,v 1.12 2006-10-29 13:00:39 bacon Exp $
 */

#include <ase/lsp/lsp_i.h>

ase_lsp_obj_t* ase_lsp_prim_plus (ase_lsp_t* lsp, ase_lsp_obj_t* args)
{
	ase_lsp_obj_t* body, * tmp;
	ase_long_t ivalue = 0;
	ase_real_t rvalue = .0;
	ase_bool_t realnum = ase_false;

	ASE_LSP_PRIM_CHECK_ARG_COUNT (lsp, args, 1, ASE_LSP_PRIM_MAX_ARG_COUNT);
	ASE_LSP_ASSERT (lsp, ASE_LSP_TYPE(args) == ASE_LSP_OBJ_CONS);

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
				ASE_LSP_ASSERT (lsp, realnum == ase_false);
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
				ASE_LSP_ASSERT (lsp, realnum == ase_false);
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

	ASE_LSP_ASSERT (lsp, body == lsp->mem->nil);

	tmp = (realnum)?
		ase_lsp_makerealobj (lsp->mem, rvalue):
		ase_lsp_makeintobj (lsp->mem, ivalue);
	if (tmp == ASE_NULL) {
		lsp->errnum = ASE_LSP_ENOMEM;
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
	ASE_LSP_ASSERT (lsp, ASE_LSP_TYPE(args) == ASE_LSP_OBJ_CONS);

	body = args;
	//while (body != lsp->mem->nil) {
	while (ASE_LSP_TYPE(body) == ASE_LSP_OBJ_CONS) {
		tmp = ase_lsp_eval (lsp, ASE_LSP_CAR(body));
		if (tmp == ASE_NULL) return ASE_NULL;


		if (ASE_LSP_TYPE(tmp) == ASE_LSP_OBJ_INT) {
			if (body == args) {
				ASE_LSP_ASSERT (lsp, realnum == ase_false);
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
				ASE_LSP_ASSERT (lsp, realnum == ase_false);
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

	ASE_LSP_ASSERT (lsp, body == lsp->mem->nil);

	tmp = (realnum)?
		ase_lsp_makerealobj (lsp->mem, rvalue):
		ase_lsp_makeintobj (lsp->mem, ivalue);
	if (tmp == ASE_NULL) {
		lsp->errnum = ASE_LSP_ENOMEM;
		return ASE_NULL;
	}

	return tmp;
}

ase_lsp_obj_t* ase_lsp_prim_mul (ase_lsp_t* lsp, ase_lsp_obj_t* args)
{
	ase_lsp_obj_t* body, * tmp;
	ase_long_t ivalue = 0;
	ase_real_t rvalue = .0;
	ase_bool_t realnum = ase_false;

	ASE_LSP_PRIM_CHECK_ARG_COUNT (lsp, args, 1, ASE_LSP_PRIM_MAX_ARG_COUNT);
	ASE_LSP_ASSERT (lsp, ASE_LSP_TYPE(args) == ASE_LSP_OBJ_CONS);

	body = args;
	//while (body != lsp->mem->nil) {
	while (ASE_LSP_TYPE(body) == ASE_LSP_OBJ_CONS) {
		tmp = ase_lsp_eval (lsp, ASE_LSP_CAR(body));
		if (tmp == ASE_NULL) return ASE_NULL;


		if (ASE_LSP_TYPE(tmp) == ASE_LSP_OBJ_INT) {
			if (body == args) {
				ASE_LSP_ASSERT (lsp, realnum == ase_false);
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
				ASE_LSP_ASSERT (lsp, realnum == ase_false);
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

	ASE_LSP_ASSERT (lsp, body == lsp->mem->nil);

	tmp = (realnum)?
		ase_lsp_makerealobj (lsp->mem, rvalue):
		ase_lsp_makeintobj (lsp->mem, ivalue);
	if (tmp == ASE_NULL) {
		lsp->errnum = ASE_LSP_ENOMEM;
		return ASE_NULL;
	}

	return tmp;
}

ase_lsp_obj_t* ase_lsp_prim_div (ase_lsp_t* lsp, ase_lsp_obj_t* args)
{
	ase_lsp_obj_t* body, * tmp;
	ase_long_t ivalue = 0;
	ase_real_t rvalue = .0;
	ase_bool_t realnum = ase_false;

	ASE_LSP_PRIM_CHECK_ARG_COUNT (lsp, args, 1, ASE_LSP_PRIM_MAX_ARG_COUNT);
	ASE_LSP_ASSERT (lsp, ASE_LSP_TYPE(args) == ASE_LSP_OBJ_CONS);

	body = args;
	//while (body != lsp->mem->nil) {
	while (ASE_LSP_TYPE(body) == ASE_LSP_OBJ_CONS) {
		tmp = ase_lsp_eval (lsp, ASE_LSP_CAR(body));
		if (tmp == ASE_NULL) return ASE_NULL;


		if (ASE_LSP_TYPE(tmp) == ASE_LSP_OBJ_INT) {
			if (body == args) {
				ASE_LSP_ASSERT (lsp, realnum == ase_false);
				ivalue = ASE_LSP_IVALUE(tmp);
			}
			else {
				if (!realnum) {
					if (ASE_LSP_IVALUE(tmp) == 0) {
						lsp->errnum = ASE_LSP_EDIVBYZERO;
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
				ASE_LSP_ASSERT (lsp, realnum == ase_false);
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

	ASE_LSP_ASSERT (lsp, body == lsp->mem->nil);

	tmp = (realnum)?
		ase_lsp_makerealobj (lsp->mem, rvalue):
		ase_lsp_makeintobj (lsp->mem, ivalue);
	if (tmp == ASE_NULL) 
	{
		lsp->errnum = ASE_LSP_ENOMEM;
		return ASE_NULL;
	}

	return tmp;
}

ase_lsp_obj_t* ase_lsp_prim_mod (ase_lsp_t* lsp, ase_lsp_obj_t* args)
{
	ase_lsp_obj_t* body, * tmp;
	ase_long_t ivalue = 0;

	ASE_LSP_PRIM_CHECK_ARG_COUNT (lsp, args, 1, ASE_LSP_PRIM_MAX_ARG_COUNT);
	ASE_LSP_ASSERT (lsp, ASE_LSP_TYPE(args) == ASE_LSP_OBJ_CONS);

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
					lsp->errnum = ASE_LSP_EDIVBYZERO;
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
					lsp->errnum = ASE_LSP_EDIVBYZERO;
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

	ASE_LSP_ASSERT (lsp, body == lsp->mem->nil);

	tmp = ase_lsp_makeintobj (lsp->mem, ivalue);
	if (tmp == ASE_NULL) 
	{
		lsp->errnum = ASE_LSP_ENOMEM;
		return ASE_NULL;
	}

	return tmp;
}
