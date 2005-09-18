/*
 * $Id: env.h,v 1.4 2005-09-18 08:10:50 bacon Exp $
 */

#ifndef _XP_LSP_ENV_H_
#define _XP_LSP_ENV_H_

#include <xp/lsp/object.h>

struct xp_lsp_assoc_t
{
	xp_lsp_obj_t*          name; // xp_lsp_obj_symbol_t
	xp_lsp_obj_t*          value;
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

xp_lsp_assoc_t* xp_lsp_assoc_new  (xp_lsp_obj_t* name, xp_lsp_obj_t* value);
void         xp_lsp_assoc_free (xp_lsp_assoc_t* assoc);

xp_lsp_frame_t* xp_lsp_frame_new    (void);
void         xp_lsp_frame_free   (xp_lsp_frame_t* frame);
xp_lsp_assoc_t* xp_lsp_frame_lookup (xp_lsp_frame_t* frame, xp_lsp_obj_t* name);
xp_lsp_assoc_t* xp_lsp_frame_insert (xp_lsp_frame_t* frame, xp_lsp_obj_t* name, xp_lsp_obj_t* value);

#ifdef __cplusplus
}
#endif

#endif
