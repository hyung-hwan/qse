/*
 * $Id: prim_prog.c,v 1.4 2006-10-26 08:17:38 bacon Exp $
 */

#include <ase/lsp/lsp_i.h>

ase_lsp_obj_t* ase_lsp_prim_prog1 (ase_lsp_t* lsp, ase_lsp_obj_t* args)
{
	ase_lsp_obj_t* res = ASE_NULL, * tmp;

	ASE_LSP_PRIM_CHECK_ARG_COUNT (lsp, args, 1, ASE_LSP_PRIM_MAX_ARG_COUNT);

	//while (args != lsp->mem->nil) {
	while (ASE_LSP_TYPE(args) == ASE_LSP_OBJ_CONS) {

		tmp = ase_lsp_eval (lsp, ASE_LSP_CAR(args));
		if (tmp == ASE_NULL) return ASE_NULL;

		if (res == ASE_NULL) {
			/*
			ase_lsp_arr_t* ta = lsp->mem->temp_arr;
			ase_lsp_arr_insert (ta, ta->size, tmp);
			*/
			res = tmp;
		}
		args = ASE_LSP_CDR(args);
	}

	return res;
}

ase_lsp_obj_t* ase_lsp_prim_progn (ase_lsp_t* lsp, ase_lsp_obj_t* args)
{
	ase_lsp_obj_t* res, * tmp;

	ASE_LSP_PRIM_CHECK_ARG_COUNT (lsp, args, 1, ASE_LSP_PRIM_MAX_ARG_COUNT);

	res = lsp->mem->nil;
	//while (args != lsp->mem->nil) {
	while (ASE_LSP_TYPE(args) == ASE_LSP_OBJ_CONS) {

		tmp = ase_lsp_eval (lsp, ASE_LSP_CAR(args));
		if (tmp == ASE_NULL) return ASE_NULL;

		res = tmp;
		args = ASE_LSP_CDR(args);
	}

	return res;
}
