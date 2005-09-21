/*
 * $Id: prim_math.c,v 1.5 2005-09-21 12:04:05 bacon Exp $
 */

#include <xp/lsp/prim.h>
#include <xp/bas/assert.h>

xp_lsp_obj_t* xp_lsp_prim_plus (xp_lsp_t* lsp, xp_lsp_obj_t* args)
{
	xp_lsp_obj_t* body, * tmp;
	xp_lsp_int_t ivalue = 0;
	xp_lsp_real_t rvalue = .0;
	xp_bool_t realnum = xp_false;

	XP_LSP_PRIM_CHECK_ARG_COUNT (lsp, args, 1, XP_LSP_PRIM_MAX_ARG_COUNT);
	xp_assert (XP_LSP_TYPE(args) == XP_LSP_OBJ_CONS);

	body = args;
	//while (body != lsp->mem->nil) {
	while (XP_LSP_TYPE(body) == XP_LSP_OBJ_CONS) {
		tmp = xp_lsp_eval (lsp, XP_LSP_CAR(body));
		if (tmp == XP_NULL) {
			/*lsp->errnum = XP_LSP_ERR_BAD_VALUE; */
			return XP_NULL;
		}

		if (XP_LSP_TYPE(tmp) == XP_LSP_OBJ_INT) {
			if (body == args) {
				xp_assert (realnum == xp_false);
				ivalue = XP_LSP_IVALUE(tmp);
			}
			else {
				if (!realnum) 
					ivalue = ivalue + XP_LSP_IVALUE(tmp);
				else
					rvalue = rvalue + XP_LSP_IVALUE(tmp);
			}
		}
		else if (XP_LSP_TYPE(tmp) == XP_LSP_OBJ_REAL) {
			if (body == args) {
				xp_assert (realnum == xp_false);
				realnum = xp_true;
				rvalue = XP_LSP_RVALUE(tmp);
			}
			else {
				if (!realnum) {
					realnum = xp_true;
					rvalue = (xp_lsp_real_t)ivalue;
				}
				rvalue = rvalue + XP_LSP_RVALUE(tmp);
			}
		}
		else {
			lsp->errnum = XP_LSP_ERR_BAD_VALUE;	
			return XP_NULL;
		}


		body = XP_LSP_CDR(body);
	}

	xp_assert (body == lsp->mem->nil);

	tmp = (realnum)?
		xp_lsp_make_real (lsp->mem, rvalue):
		xp_lsp_make_int (lsp->mem, ivalue);
	if (tmp == XP_NULL) {
		lsp->errnum = XP_LSP_ERR_MEM;
		return XP_NULL;
	}

	return tmp;
}

xp_lsp_obj_t* xp_lsp_prim_minus (xp_lsp_t* lsp, xp_lsp_obj_t* args)
{
	xp_lsp_obj_t* body, * tmp;
	xp_lsp_int_t ivalue = 0;
	xp_lsp_real_t rvalue = .0;
	xp_bool_t realnum = xp_false;

	XP_LSP_PRIM_CHECK_ARG_COUNT (lsp, args, 1, XP_LSP_PRIM_MAX_ARG_COUNT);
	xp_assert (XP_LSP_TYPE(args) == XP_LSP_OBJ_CONS);

	body = args;
	//while (body != lsp->mem->nil) {
	while (XP_LSP_TYPE(body) == XP_LSP_OBJ_CONS) {
		tmp = xp_lsp_eval (lsp, XP_LSP_CAR(body));
		if (tmp == XP_NULL) return XP_NULL;


		if (XP_LSP_TYPE(tmp) == XP_LSP_OBJ_INT) {
			if (body == args) {
				xp_assert (realnum == xp_false);
				ivalue = XP_LSP_IVALUE(tmp);
			}
			else {
				if (!realnum) 
					ivalue = ivalue - XP_LSP_IVALUE(tmp);
				else
					rvalue = rvalue - XP_LSP_IVALUE(tmp);
			}
		}
		else if (XP_LSP_TYPE(tmp) == XP_LSP_OBJ_REAL) {
			if (body == args) {
				xp_assert (realnum == xp_false);
				realnum = xp_true;
				rvalue = XP_LSP_RVALUE(tmp);
			}
			else {
				if (!realnum) {
					realnum = xp_true;
					rvalue = (xp_lsp_real_t)ivalue;
				}
				rvalue = rvalue - XP_LSP_RVALUE(tmp);
			}
		}
		else {
			lsp->errnum = XP_LSP_ERR_BAD_VALUE;	
			return XP_NULL;
		}


		body = XP_LSP_CDR(body);
	}

	xp_assert (body == lsp->mem->nil);

	tmp = (realnum)?
		xp_lsp_make_real (lsp->mem, rvalue):
		xp_lsp_make_int (lsp->mem, ivalue);
	if (tmp == XP_NULL) {
		lsp->errnum = XP_LSP_ERR_MEM;
		return XP_NULL;
	}

	return tmp;
}

xp_lsp_obj_t* xp_lsp_prim_multiply (xp_lsp_t* lsp, xp_lsp_obj_t* args)
{
	xp_lsp_obj_t* body, * tmp;
	xp_lsp_int_t ivalue = 0;
	xp_lsp_real_t rvalue = .0;
	xp_bool_t realnum = xp_false;

	XP_LSP_PRIM_CHECK_ARG_COUNT (lsp, args, 1, XP_LSP_PRIM_MAX_ARG_COUNT);
	xp_assert (XP_LSP_TYPE(args) == XP_LSP_OBJ_CONS);

	body = args;
	//while (body != lsp->mem->nil) {
	while (XP_LSP_TYPE(body) == XP_LSP_OBJ_CONS) {
		tmp = xp_lsp_eval (lsp, XP_LSP_CAR(body));
		if (tmp == XP_NULL) return XP_NULL;


		if (XP_LSP_TYPE(tmp) == XP_LSP_OBJ_INT) {
			if (body == args) {
				xp_assert (realnum == xp_false);
				ivalue = XP_LSP_IVALUE(tmp);
			}
			else {
				if (!realnum) 
					ivalue = ivalue * XP_LSP_IVALUE(tmp);
				else
					rvalue = rvalue * XP_LSP_IVALUE(tmp);
			}
		}
		else if (XP_LSP_TYPE(tmp) == XP_LSP_OBJ_REAL) {
			if (body == args) {
				xp_assert (realnum == xp_false);
				realnum = xp_true;
				rvalue = XP_LSP_RVALUE(tmp);
			}
			else {
				if (!realnum) {
					realnum = xp_true;
					rvalue = (xp_lsp_real_t)ivalue;
				}
				rvalue = rvalue * XP_LSP_RVALUE(tmp);
			}
		}
		else {
			lsp->errnum = XP_LSP_ERR_BAD_VALUE;	
			return XP_NULL;
		}


		body = XP_LSP_CDR(body);
	}

	xp_assert (body == lsp->mem->nil);

	tmp = (realnum)?
		xp_lsp_make_real (lsp->mem, rvalue):
		xp_lsp_make_int (lsp->mem, ivalue);
	if (tmp == XP_NULL) {
		lsp->errnum = XP_LSP_ERR_MEM;
		return XP_NULL;
	}

	return tmp;
}

xp_lsp_obj_t* xp_lsp_prim_divide (xp_lsp_t* lsp, xp_lsp_obj_t* args)
{
	xp_lsp_obj_t* body, * tmp;
	xp_lsp_int_t ivalue = 0;
	xp_lsp_real_t rvalue = .0;
	xp_bool_t realnum = xp_false;

	XP_LSP_PRIM_CHECK_ARG_COUNT (lsp, args, 1, XP_LSP_PRIM_MAX_ARG_COUNT);
	xp_assert (XP_LSP_TYPE(args) == XP_LSP_OBJ_CONS);

	body = args;
	//while (body != lsp->mem->nil) {
	while (XP_LSP_TYPE(body) == XP_LSP_OBJ_CONS) {
		tmp = xp_lsp_eval (lsp, XP_LSP_CAR(body));
		if (tmp == XP_NULL) return XP_NULL;


		if (XP_LSP_TYPE(tmp) == XP_LSP_OBJ_INT) {
			if (body == args) {
				xp_assert (realnum == xp_false);
				ivalue = XP_LSP_IVALUE(tmp);
			}
			else {
				if (!realnum) {
					if (XP_LSP_IVALUE(tmp) == 0) {
						lsp->errnum = XP_LSP_ERR_DIVIDE_BY_ZERO;
						return XP_NULL;
					}
					ivalue = ivalue / XP_LSP_IVALUE(tmp);
				}
				else
					rvalue = rvalue / XP_LSP_IVALUE(tmp);
			}
		}
		else if (XP_LSP_TYPE(tmp) == XP_LSP_OBJ_REAL) {
			if (body == args) {
				xp_assert (realnum == xp_false);
				realnum = xp_true;
				rvalue = XP_LSP_RVALUE(tmp);
			}
			else {
				if (!realnum) {
					realnum = xp_true;
					rvalue = (xp_lsp_real_t)ivalue;
				}
				rvalue = rvalue / XP_LSP_RVALUE(tmp);
			}
		}
		else {
			lsp->errnum = XP_LSP_ERR_BAD_VALUE;	
			return XP_NULL;
		}


		body = XP_LSP_CDR(body);
	}

	xp_assert (body == lsp->mem->nil);

	tmp = (realnum)?
		xp_lsp_make_real (lsp->mem, rvalue):
		xp_lsp_make_int (lsp->mem, ivalue);
	if (tmp == XP_NULL) {
		lsp->errnum = XP_LSP_ERR_MEM;
		return XP_NULL;
	}

	return tmp;
}

xp_lsp_obj_t* xp_lsp_prim_modulus (xp_lsp_t* lsp, xp_lsp_obj_t* args)
{
	xp_lsp_obj_t* body, * tmp;
	xp_lsp_int_t ivalue = 0;

	XP_LSP_PRIM_CHECK_ARG_COUNT (lsp, args, 1, XP_LSP_PRIM_MAX_ARG_COUNT);
	xp_assert (XP_LSP_TYPE(args) == XP_LSP_OBJ_CONS);

	body = args;
	//while (body != lsp->mem->nil) {
	while (XP_LSP_TYPE(body) == XP_LSP_OBJ_CONS) {
		tmp = xp_lsp_eval (lsp, XP_LSP_CAR(body));
		if (tmp == XP_NULL) return XP_NULL;

		if (XP_LSP_TYPE(tmp) == XP_LSP_OBJ_INT) {
			if (body == args) {
				ivalue = XP_LSP_IVALUE(tmp);
			}
			else {
				if (XP_LSP_IVALUE(tmp) == 0) {
					lsp->errnum = XP_LSP_ERR_DIVIDE_BY_ZERO;
					return XP_NULL;
				}
				ivalue = ivalue % XP_LSP_IVALUE(tmp);
			}
		}
		else if (XP_LSP_TYPE(tmp) == XP_LSP_OBJ_REAL) {
			if (body == args) {
				ivalue = (xp_lsp_int_t)XP_LSP_RVALUE(tmp);
			}
			else {
				xp_lsp_int_t tmpi = (xp_lsp_int_t)XP_LSP_RVALUE(tmp);
				if (tmpi == 0) {
					lsp->errnum = XP_LSP_ERR_DIVIDE_BY_ZERO;
					return XP_NULL;
				}
				ivalue = ivalue % tmpi;
			}
		}
		else {
			lsp->errnum = XP_LSP_ERR_BAD_VALUE;	
			return XP_NULL;
		}


		body = XP_LSP_CDR(body);
	}

	xp_assert (body == lsp->mem->nil);

	tmp = xp_lsp_make_int (lsp->mem, ivalue);
	if (tmp == XP_NULL) {
		lsp->errnum = XP_LSP_ERR_MEM;
		return XP_NULL;
	}

	return tmp;
}
