/*
 * $Id: prim_math.c,v 1.7 2006-10-22 13:10:46 bacon Exp $
 */

#include <sse/lsp/prim.h>
#include <sse/bas/assert.h>

sse_lsp_obj_t* sse_lsp_prim_plus (sse_lsp_t* lsp, sse_lsp_obj_t* args)
{
	sse_lsp_obj_t* body, * tmp;
	sse_lsp_int_t ivalue = 0;
	sse_lsp_real_t rvalue = .0;
	sse_bool_t realnum = sse_false;

	SSE_LSP_PRIM_CHECK_ARG_COUNT (lsp, args, 1, SSE_LSP_PRIM_MAX_ARG_COUNT);
	sse_assert (SSE_LSP_TYPE(args) == SSE_LSP_OBJ_CONS);

	body = args;
	//while (body != lsp->mem->nil) {
	while (SSE_LSP_TYPE(body) == SSE_LSP_OBJ_CONS) {
		tmp = sse_lsp_eval (lsp, SSE_LSP_CAR(body));
		if (tmp == SSE_NULL) {
			/*lsp->errnum = SSE_LSP_ERR_BAD_VALUE; */
			return SSE_NULL;
		}

		if (SSE_LSP_TYPE(tmp) == SSE_LSP_OBJ_INT) {
			if (body == args) {
				sse_assert (realnum == sse_false);
				ivalue = SSE_LSP_IVALUE(tmp);
			}
			else {
				if (!realnum) 
					ivalue = ivalue + SSE_LSP_IVALUE(tmp);
				else
					rvalue = rvalue + SSE_LSP_IVALUE(tmp);
			}
		}
		else if (SSE_LSP_TYPE(tmp) == SSE_LSP_OBJ_REAL) {
			if (body == args) {
				sse_assert (realnum == sse_false);
				realnum = sse_true;
				rvalue = SSE_LSP_RVALUE(tmp);
			}
			else {
				if (!realnum) {
					realnum = sse_true;
					rvalue = (sse_lsp_real_t)ivalue;
				}
				rvalue = rvalue + SSE_LSP_RVALUE(tmp);
			}
		}
		else {
			lsp->errnum = SSE_LSP_ERR_BAD_VALUE;	
			return SSE_NULL;
		}


		body = SSE_LSP_CDR(body);
	}

	sse_assert (body == lsp->mem->nil);

	tmp = (realnum)?
		sse_lsp_make_real (lsp->mem, rvalue):
		sse_lsp_make_int (lsp->mem, ivalue);
	if (tmp == SSE_NULL) {
		lsp->errnum = SSE_LSP_ERR_MEMORY;
		return SSE_NULL;
	}

	return tmp;
}

sse_lsp_obj_t* sse_lsp_prim_minus (sse_lsp_t* lsp, sse_lsp_obj_t* args)
{
	sse_lsp_obj_t* body, * tmp;
	sse_lsp_int_t ivalue = 0;
	sse_lsp_real_t rvalue = .0;
	sse_bool_t realnum = sse_false;

	SSE_LSP_PRIM_CHECK_ARG_COUNT (lsp, args, 1, SSE_LSP_PRIM_MAX_ARG_COUNT);
	sse_assert (SSE_LSP_TYPE(args) == SSE_LSP_OBJ_CONS);

	body = args;
	//while (body != lsp->mem->nil) {
	while (SSE_LSP_TYPE(body) == SSE_LSP_OBJ_CONS) {
		tmp = sse_lsp_eval (lsp, SSE_LSP_CAR(body));
		if (tmp == SSE_NULL) return SSE_NULL;


		if (SSE_LSP_TYPE(tmp) == SSE_LSP_OBJ_INT) {
			if (body == args) {
				sse_assert (realnum == sse_false);
				ivalue = SSE_LSP_IVALUE(tmp);
			}
			else {
				if (!realnum) 
					ivalue = ivalue - SSE_LSP_IVALUE(tmp);
				else
					rvalue = rvalue - SSE_LSP_IVALUE(tmp);
			}
		}
		else if (SSE_LSP_TYPE(tmp) == SSE_LSP_OBJ_REAL) {
			if (body == args) {
				sse_assert (realnum == sse_false);
				realnum = sse_true;
				rvalue = SSE_LSP_RVALUE(tmp);
			}
			else {
				if (!realnum) {
					realnum = sse_true;
					rvalue = (sse_lsp_real_t)ivalue;
				}
				rvalue = rvalue - SSE_LSP_RVALUE(tmp);
			}
		}
		else {
			lsp->errnum = SSE_LSP_ERR_BAD_VALUE;	
			return SSE_NULL;
		}


		body = SSE_LSP_CDR(body);
	}

	sse_assert (body == lsp->mem->nil);

	tmp = (realnum)?
		sse_lsp_make_real (lsp->mem, rvalue):
		sse_lsp_make_int (lsp->mem, ivalue);
	if (tmp == SSE_NULL) {
		lsp->errnum = SSE_LSP_ERR_MEMORY;
		return SSE_NULL;
	}

	return tmp;
}

sse_lsp_obj_t* sse_lsp_prim_multiply (sse_lsp_t* lsp, sse_lsp_obj_t* args)
{
	sse_lsp_obj_t* body, * tmp;
	sse_lsp_int_t ivalue = 0;
	sse_lsp_real_t rvalue = .0;
	sse_bool_t realnum = sse_false;

	SSE_LSP_PRIM_CHECK_ARG_COUNT (lsp, args, 1, SSE_LSP_PRIM_MAX_ARG_COUNT);
	sse_assert (SSE_LSP_TYPE(args) == SSE_LSP_OBJ_CONS);

	body = args;
	//while (body != lsp->mem->nil) {
	while (SSE_LSP_TYPE(body) == SSE_LSP_OBJ_CONS) {
		tmp = sse_lsp_eval (lsp, SSE_LSP_CAR(body));
		if (tmp == SSE_NULL) return SSE_NULL;


		if (SSE_LSP_TYPE(tmp) == SSE_LSP_OBJ_INT) {
			if (body == args) {
				sse_assert (realnum == sse_false);
				ivalue = SSE_LSP_IVALUE(tmp);
			}
			else {
				if (!realnum) 
					ivalue = ivalue * SSE_LSP_IVALUE(tmp);
				else
					rvalue = rvalue * SSE_LSP_IVALUE(tmp);
			}
		}
		else if (SSE_LSP_TYPE(tmp) == SSE_LSP_OBJ_REAL) {
			if (body == args) {
				sse_assert (realnum == sse_false);
				realnum = sse_true;
				rvalue = SSE_LSP_RVALUE(tmp);
			}
			else {
				if (!realnum) {
					realnum = sse_true;
					rvalue = (sse_lsp_real_t)ivalue;
				}
				rvalue = rvalue * SSE_LSP_RVALUE(tmp);
			}
		}
		else {
			lsp->errnum = SSE_LSP_ERR_BAD_VALUE;	
			return SSE_NULL;
		}


		body = SSE_LSP_CDR(body);
	}

	sse_assert (body == lsp->mem->nil);

	tmp = (realnum)?
		sse_lsp_make_real (lsp->mem, rvalue):
		sse_lsp_make_int (lsp->mem, ivalue);
	if (tmp == SSE_NULL) {
		lsp->errnum = SSE_LSP_ERR_MEMORY;
		return SSE_NULL;
	}

	return tmp;
}

sse_lsp_obj_t* sse_lsp_prim_divide (sse_lsp_t* lsp, sse_lsp_obj_t* args)
{
	sse_lsp_obj_t* body, * tmp;
	sse_lsp_int_t ivalue = 0;
	sse_lsp_real_t rvalue = .0;
	sse_bool_t realnum = sse_false;

	SSE_LSP_PRIM_CHECK_ARG_COUNT (lsp, args, 1, SSE_LSP_PRIM_MAX_ARG_COUNT);
	sse_assert (SSE_LSP_TYPE(args) == SSE_LSP_OBJ_CONS);

	body = args;
	//while (body != lsp->mem->nil) {
	while (SSE_LSP_TYPE(body) == SSE_LSP_OBJ_CONS) {
		tmp = sse_lsp_eval (lsp, SSE_LSP_CAR(body));
		if (tmp == SSE_NULL) return SSE_NULL;


		if (SSE_LSP_TYPE(tmp) == SSE_LSP_OBJ_INT) {
			if (body == args) {
				sse_assert (realnum == sse_false);
				ivalue = SSE_LSP_IVALUE(tmp);
			}
			else {
				if (!realnum) {
					if (SSE_LSP_IVALUE(tmp) == 0) {
						lsp->errnum = SSE_LSP_ERR_DIVIDE_BY_ZERO;
						return SSE_NULL;
					}
					ivalue = ivalue / SSE_LSP_IVALUE(tmp);
				}
				else
					rvalue = rvalue / SSE_LSP_IVALUE(tmp);
			}
		}
		else if (SSE_LSP_TYPE(tmp) == SSE_LSP_OBJ_REAL) {
			if (body == args) {
				sse_assert (realnum == sse_false);
				realnum = sse_true;
				rvalue = SSE_LSP_RVALUE(tmp);
			}
			else {
				if (!realnum) {
					realnum = sse_true;
					rvalue = (sse_lsp_real_t)ivalue;
				}
				rvalue = rvalue / SSE_LSP_RVALUE(tmp);
			}
		}
		else {
			lsp->errnum = SSE_LSP_ERR_BAD_VALUE;	
			return SSE_NULL;
		}


		body = SSE_LSP_CDR(body);
	}

	sse_assert (body == lsp->mem->nil);

	tmp = (realnum)?
		sse_lsp_make_real (lsp->mem, rvalue):
		sse_lsp_make_int (lsp->mem, ivalue);
	if (tmp == SSE_NULL) {
		lsp->errnum = SSE_LSP_ERR_MEMORY;
		return SSE_NULL;
	}

	return tmp;
}

sse_lsp_obj_t* sse_lsp_prim_modulus (sse_lsp_t* lsp, sse_lsp_obj_t* args)
{
	sse_lsp_obj_t* body, * tmp;
	sse_lsp_int_t ivalue = 0;

	SSE_LSP_PRIM_CHECK_ARG_COUNT (lsp, args, 1, SSE_LSP_PRIM_MAX_ARG_COUNT);
	sse_assert (SSE_LSP_TYPE(args) == SSE_LSP_OBJ_CONS);

	body = args;
	//while (body != lsp->mem->nil) {
	while (SSE_LSP_TYPE(body) == SSE_LSP_OBJ_CONS) {
		tmp = sse_lsp_eval (lsp, SSE_LSP_CAR(body));
		if (tmp == SSE_NULL) return SSE_NULL;

		if (SSE_LSP_TYPE(tmp) == SSE_LSP_OBJ_INT) {
			if (body == args) {
				ivalue = SSE_LSP_IVALUE(tmp);
			}
			else {
				if (SSE_LSP_IVALUE(tmp) == 0) {
					lsp->errnum = SSE_LSP_ERR_DIVIDE_BY_ZERO;
					return SSE_NULL;
				}
				ivalue = ivalue % SSE_LSP_IVALUE(tmp);
			}
		}
		else if (SSE_LSP_TYPE(tmp) == SSE_LSP_OBJ_REAL) {
			if (body == args) {
				ivalue = (sse_lsp_int_t)SSE_LSP_RVALUE(tmp);
			}
			else {
				sse_lsp_int_t tmpi = (sse_lsp_int_t)SSE_LSP_RVALUE(tmp);
				if (tmpi == 0) {
					lsp->errnum = SSE_LSP_ERR_DIVIDE_BY_ZERO;
					return SSE_NULL;
				}
				ivalue = ivalue % tmpi;
			}
		}
		else {
			lsp->errnum = SSE_LSP_ERR_BAD_VALUE;	
			return SSE_NULL;
		}


		body = SSE_LSP_CDR(body);
	}

	sse_assert (body == lsp->mem->nil);

	tmp = sse_lsp_make_int (lsp->mem, ivalue);
	if (tmp == SSE_NULL) {
		lsp->errnum = SSE_LSP_ERR_MEMORY;
		return SSE_NULL;
	}

	return tmp;
}
