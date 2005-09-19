/*
 * $Id: prim_prog.c,v 1.1 2005-09-19 12:04:00 bacon Exp $
 */

#include <xp/lsp/prim.h>

xp_lsp_obj_t* xp_lsp_prim_prog1 (xp_lsp_t* lsp, xp_lsp_obj_t* args)
{
	xp_lsp_obj_t* res = XP_NULL, * tmp;

	XP_LSP_PRIM_CHECK_ARG_COUNT (lsp, args, 1, XP_LSP_PRIM_MAX_ARG_COUNT);

	//while (args != lsp->mem->nil) {
	while (XP_LSP_TYPE(args) == XP_LSP_OBJ_CONS) {

		tmp = xp_lsp_eval (lsp, XP_LSP_CAR(args));
		if (tmp == XP_NULL) return XP_NULL;

		if (res == XP_NULL) {
			/*
			xp_lsp_array_t* ta = lsp->mem->temp_array;
			xp_lsp_array_insert (ta, ta->size, tmp);
			*/
			res = tmp;
		}
		args = XP_LSP_CDR(args);
	}

	return res;
}

xp_lsp_obj_t* xp_lsp_prim_progn (xp_lsp_t* lsp, xp_lsp_obj_t* args)
{
	xp_lsp_obj_t* res, * tmp;

	XP_LSP_PRIM_CHECK_ARG_COUNT (lsp, args, 1, XP_LSP_PRIM_MAX_ARG_COUNT);

	res = lsp->mem->nil;
	//while (args != lsp->mem->nil) {
	while (XP_LSP_TYPE(args) == XP_LSP_OBJ_CONS) {

		tmp = xp_lsp_eval (lsp, XP_LSP_CAR(args));
		if (tmp == XP_NULL) return XP_NULL;

		res = tmp;
		args = XP_LSP_CDR(args);
	}

	return res;
}
