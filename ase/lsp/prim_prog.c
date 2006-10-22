/*
 * $Id: prim_prog.c,v 1.2 2006-10-22 13:10:46 bacon Exp $
 */

#include <sse/lsp/prim.h>

sse_lsp_obj_t* sse_lsp_prim_prog1 (sse_lsp_t* lsp, sse_lsp_obj_t* args)
{
	sse_lsp_obj_t* res = SSE_NULL, * tmp;

	SSE_LSP_PRIM_CHECK_ARG_COUNT (lsp, args, 1, SSE_LSP_PRIM_MAX_ARG_COUNT);

	//while (args != lsp->mem->nil) {
	while (SSE_LSP_TYPE(args) == SSE_LSP_OBJ_CONS) {

		tmp = sse_lsp_eval (lsp, SSE_LSP_CAR(args));
		if (tmp == SSE_NULL) return SSE_NULL;

		if (res == SSE_NULL) {
			/*
			sse_lsp_array_t* ta = lsp->mem->temp_array;
			sse_lsp_array_insert (ta, ta->size, tmp);
			*/
			res = tmp;
		}
		args = SSE_LSP_CDR(args);
	}

	return res;
}

sse_lsp_obj_t* sse_lsp_prim_progn (sse_lsp_t* lsp, sse_lsp_obj_t* args)
{
	sse_lsp_obj_t* res, * tmp;

	SSE_LSP_PRIM_CHECK_ARG_COUNT (lsp, args, 1, SSE_LSP_PRIM_MAX_ARG_COUNT);

	res = lsp->mem->nil;
	//while (args != lsp->mem->nil) {
	while (SSE_LSP_TYPE(args) == SSE_LSP_OBJ_CONS) {

		tmp = sse_lsp_eval (lsp, SSE_LSP_CAR(args));
		if (tmp == SSE_NULL) return SSE_NULL;

		res = tmp;
		args = SSE_LSP_CDR(args);
	}

	return res;
}
