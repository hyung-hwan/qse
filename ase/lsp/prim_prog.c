/*
 * $Id: prim_prog.c,v 1.3 2006-10-24 04:22:39 bacon Exp $
 */

#include <ase/lsp/prim.h>

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
			ase_lsp_array_t* ta = lsp->mem->temp_array;
			ase_lsp_array_insert (ta, ta->size, tmp);
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
