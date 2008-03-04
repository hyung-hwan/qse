/*
 * $Id: prim_compar.c 117 2008-03-03 11:20:05Z baconevi $
 *
 * {License}
 */

#include <ase/lsp/lsp_i.h>

#define PRIM_COMPAR(lsp,args,op)  \
{ \
	ase_lsp_obj_t* p1, * p2; \
	int res; \
	ASE_ASSERT (ASE_LSP_TYPE(args) == ASE_LSP_OBJ_CONS); \
\
	p1 = ase_lsp_eval (lsp, ASE_LSP_CAR(args)); \
	if (p1 == ASE_NULL) return ASE_NULL; \
	if (ase_lsp_pushtmp (lsp, p1) == ASE_NULL) return ASE_NULL; \
\
	p2 = ase_lsp_eval (lsp, ASE_LSP_CAR(ASE_LSP_CDR(args))); \
	if (p2 == ASE_NULL) \
	{ \
		ase_lsp_poptmp (lsp); \
		return ASE_NULL; \
	} \
\
	if (ase_lsp_pushtmp (lsp, p2) == ASE_NULL) \
	{ \
		ase_lsp_poptmp (lsp); \
		return ASE_NULL; \
	} \
\
	if (ASE_LSP_TYPE(p1) == ASE_LSP_OBJ_INT) \
	{ \
		if (ASE_LSP_TYPE(p2) == ASE_LSP_OBJ_INT) \
		{ \
			res = ASE_LSP_IVAL(p1) op ASE_LSP_IVAL(p2); \
		} \
		else if (ASE_LSP_TYPE(p2) == ASE_LSP_OBJ_REAL) \
		{ \
			res = ASE_LSP_IVAL(p1) op ASE_LSP_RVAL(p2); \
		} \
		else \
		{ \
			ase_lsp_poptmp (lsp); \
			ase_lsp_poptmp (lsp); \
			ase_lsp_seterror (lsp, ASE_LSP_EVALBAD, ASE_NULL, 0); \
			return ASE_NULL; \
		} \
	} \
	else if (ASE_LSP_TYPE(p1) == ASE_LSP_OBJ_REAL) \
	{ \
		if (ASE_LSP_TYPE(p2) == ASE_LSP_OBJ_INT) \
		{ \
			res = ASE_LSP_RVAL(p1) op ASE_LSP_IVAL(p2); \
		} \
		else if (ASE_LSP_TYPE(p2) == ASE_LSP_OBJ_REAL) \
		{ \
			res = ASE_LSP_RVAL(p1) op ASE_LSP_RVAL(p2); \
		} \
		else \
		{ \
			ase_lsp_poptmp (lsp); \
			ase_lsp_poptmp (lsp); \
			ase_lsp_seterror (lsp, ASE_LSP_EVALBAD, ASE_NULL, 0); \
			return ASE_NULL; \
		} \
	} \
	else if (ASE_LSP_TYPE(p1) == ASE_LSP_OBJ_SYM) \
	{ \
		if (ASE_LSP_TYPE(p2) == ASE_LSP_OBJ_SYM) \
		{ \
			res = ase_strxncmp ( \
				ASE_LSP_SYMPTR(p1), ASE_LSP_SYMLEN(p1), \
				ASE_LSP_SYMPTR(p2), ASE_LSP_SYMLEN(p2)) op 0; \
		} \
		else  \
		{ \
			ase_lsp_poptmp (lsp); \
			ase_lsp_poptmp (lsp); \
			ase_lsp_seterror (lsp, ASE_LSP_EVALBAD, ASE_NULL, 0); \
			return ASE_NULL; \
		} \
	} \
	else if (ASE_LSP_TYPE(p1) == ASE_LSP_OBJ_STR) \
	{ \
		if (ASE_LSP_TYPE(p2) == ASE_LSP_OBJ_STR) \
		{ \
			res = ase_strxncmp ( \
				ASE_LSP_STRPTR(p1), ASE_LSP_STRLEN(p1),	\
				ASE_LSP_STRPTR(p2), ASE_LSP_STRLEN(p2)) op 0; \
		} \
		else \
		{ \
			ase_lsp_poptmp (lsp); \
			ase_lsp_poptmp (lsp); \
			ase_lsp_seterror (lsp, ASE_LSP_EVALBAD, ASE_NULL, 0); \
			return ASE_NULL; \
		} \
	} \
	else \
	{ \
		ase_lsp_poptmp (lsp); \
		ase_lsp_poptmp (lsp); \
		ase_lsp_seterror (lsp, ASE_LSP_EVALBAD, ASE_NULL, 0); \
		return ASE_NULL; \
	} \
\
	ase_lsp_poptmp (lsp); \
	ase_lsp_poptmp (lsp); \
	return (res)? lsp->mem->t: lsp->mem->nil; \
}

ase_lsp_obj_t* ase_lsp_prim_eq (ase_lsp_t* lsp, ase_lsp_obj_t* args)
{
	PRIM_COMPAR (lsp, args, ==);
}

ase_lsp_obj_t* ase_lsp_prim_ne (ase_lsp_t* lsp, ase_lsp_obj_t* args)
{
	PRIM_COMPAR (lsp, args, !=);
}

ase_lsp_obj_t* ase_lsp_prim_gt (ase_lsp_t* lsp, ase_lsp_obj_t* args)
{
	PRIM_COMPAR (lsp, args, >);
}

ase_lsp_obj_t* ase_lsp_prim_lt (ase_lsp_t* lsp, ase_lsp_obj_t* args)
{
	PRIM_COMPAR (lsp, args, <);
}

ase_lsp_obj_t* ase_lsp_prim_ge (ase_lsp_t* lsp, ase_lsp_obj_t* args)
{
	PRIM_COMPAR (lsp, args, >=);
}

ase_lsp_obj_t* ase_lsp_prim_le (ase_lsp_t* lsp, ase_lsp_obj_t* args)
{
	PRIM_COMPAR (lsp, args, <=);
}
