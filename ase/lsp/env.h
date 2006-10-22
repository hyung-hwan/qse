/*
 * $Id: env.h,v 1.8 2006-10-22 13:10:45 bacon Exp $
 */

#ifndef _SSE_LSP_ENV_H_
#define _SSE_LSP_ENV_H_

#include <sse/lsp/obj.h>

struct sse_lsp_assoc_t
{
	sse_lsp_obj_t* name; // sse_lsp_obj_symbol_t
	/*sse_lsp_obj_t* value;*/
	sse_lsp_obj_t* value; /* value as a variable */
	sse_lsp_obj_t* func;  /* function definition */
	struct sse_lsp_assoc_t* link;
};

struct sse_lsp_frame_t
{
	struct sse_lsp_assoc_t* assoc;
	struct sse_lsp_frame_t* link;
};

typedef struct sse_lsp_assoc_t sse_lsp_assoc_t;
typedef struct sse_lsp_frame_t sse_lsp_frame_t;

#ifdef __cplusplus
extern "C" {
#endif

sse_lsp_assoc_t* sse_lsp_assoc_new (
	sse_lsp_obj_t* name, sse_lsp_obj_t* value, sse_lsp_obj_t* func);
void sse_lsp_assoc_free (sse_lsp_assoc_t* assoc);

sse_lsp_frame_t* sse_lsp_frame_new (void);
void sse_lsp_frame_free (sse_lsp_frame_t* frame);
sse_lsp_assoc_t* sse_lsp_frame_lookup (sse_lsp_frame_t* frame, sse_lsp_obj_t* name);

sse_lsp_assoc_t* sse_lsp_frame_insert_value (
	sse_lsp_frame_t* frame, sse_lsp_obj_t* name, sse_lsp_obj_t* value);
sse_lsp_assoc_t* sse_lsp_frame_insert_func (
	sse_lsp_frame_t* frame, sse_lsp_obj_t* name, sse_lsp_obj_t* func);

#ifdef __cplusplus
}
#endif

#endif
