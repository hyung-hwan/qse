/*
 * $Id: prim_prog.c,v 1.3 2007/04/30 06:09:46 bacon Exp $
 *
 * {License}
 */

#include <ase/lsp/lsp_i.h>

ase_lsp_obj_t* ase_lsp_prim_prog1 (ase_lsp_t* lsp, ase_lsp_obj_t* args)
{
	/* (prog1 1 2 3) returns 1 */
	ase_lsp_obj_t* res = ASE_NULL, * tmp;

	/*while (args != lsp->mem->nil) {*/
	while (ASE_LSP_TYPE(args) == ASE_LSP_OBJ_CONS) 
	{
		tmp = ase_lsp_eval (lsp, ASE_LSP_CAR(args));
		if (tmp == ASE_NULL) return ASE_NULL;

		if (res == ASE_NULL) 
		{
			res = tmp;
			if (ase_lsp_pushtmp (lsp, res) == ASE_NULL)
			{
				return ASE_NULL;
			}
		}
		args = ASE_LSP_CDR(args);
	}

	if (res != ASE_NULL) ase_lsp_poptmp (lsp);
	return res;
}

ase_lsp_obj_t* ase_lsp_prim_progn (ase_lsp_t* lsp, ase_lsp_obj_t* args)
{
	/* (progn 1 2 3) returns 3 */

	ase_lsp_obj_t* res, * tmp;

	res = lsp->mem->nil;
	/*while (args != lsp->mem->nil) {*/
	while (ASE_LSP_TYPE(args) == ASE_LSP_OBJ_CONS) 
	{
		tmp = ase_lsp_eval (lsp, ASE_LSP_CAR(args));
		if (tmp == ASE_NULL) return ASE_NULL;

		res = tmp;
		args = ASE_LSP_CDR(args);
	}

	return res;
}
