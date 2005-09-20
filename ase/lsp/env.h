/*
 * $Id: env.h,v 1.7 2005-09-20 09:17:06 bacon Exp $
 */

#ifndef _XP_LSP_ENV_H_
#define _XP_LSP_ENV_H_

#include <xp/lsp/obj.h>

struct xp_lsp_assoc_t
{
	xp_lsp_obj_t* name; // xp_lsp_obj_symbol_t
	/*xp_lsp_obj_t* value;*/
	xp_lsp_obj_t* value; /* value as a variable */
	xp_lsp_obj_t* func;  /* function definition */
	struct xp_lsp_assoc_t* link;
};

struct xp_lsp_frame_t
{
	struct xp_lsp_assoc_t* assoc;
	struct xp_lsp_frame_t* link;
};

typedef struct xp_lsp_assoc_t xp_lsp_assoc_t;
typedef struct xp_lsp_frame_t xp_lsp_frame_t;

#ifdef __cplusplus
extern "C" {
#endif

xp_lsp_assoc_t* xp_lsp_assoc_new (
	xp_lsp_obj_t* name, xp_lsp_obj_t* value, xp_lsp_obj_t* func);
void xp_lsp_assoc_free (xp_lsp_assoc_t* assoc);

xp_lsp_frame_t* xp_lsp_frame_new (void);
void xp_lsp_frame_free (xp_lsp_frame_t* frame);
xp_lsp_assoc_t* xp_lsp_frame_lookup (xp_lsp_frame_t* frame, xp_lsp_obj_t* name);

xp_lsp_assoc_t* xp_lsp_frame_insert_value (
	xp_lsp_frame_t* frame, xp_lsp_obj_t* name, xp_lsp_obj_t* value);
xp_lsp_assoc_t* xp_lsp_frame_insert_func (
	xp_lsp_frame_t* frame, xp_lsp_obj_t* name, xp_lsp_obj_t* func);

#ifdef __cplusplus
}
#endif

#endif
