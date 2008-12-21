/*
 * $Id: env.h 117 2008-03-03 11:20:05Z baconevi $
 *
 * {License}
 */

#ifndef _QSE_LSP_ENV_H_
#define _QSE_LSP_ENV_H_

#ifndef _QSE_LSP_LSP_H_
#error Never include this file directly. Include <qse/lsp/lsp.h> instead
#endif

typedef struct qse_lsp_assoc_t qse_lsp_assoc_t;
typedef struct qse_lsp_frame_t qse_lsp_frame_t;
typedef struct qse_lsp_tlink_t qse_lsp_tlink_t;

struct qse_lsp_assoc_t
{
	qse_lsp_obj_t* name;  /* qse_lsp_obj_sym_t */
	qse_lsp_obj_t* value; /* value as a variable */
	qse_lsp_obj_t* func;  /* function definition */

	qse_lsp_assoc_t* link;
};

struct qse_lsp_frame_t
{
	qse_lsp_assoc_t* assoc;
	qse_lsp_frame_t* link;
};

struct qse_lsp_tlink_t
{
	qse_lsp_obj_t*   obj;
	qse_lsp_tlink_t* link;
};

#ifdef __cplusplus
extern "C" {
#endif

qse_lsp_frame_t* qse_lsp_newframe (qse_lsp_t* lsp);
void qse_lsp_freeframe (qse_lsp_t* lsp, qse_lsp_frame_t* frame);

qse_lsp_assoc_t* qse_lsp_lookupinframe (
	qse_lsp_t* lsp, qse_lsp_frame_t* frame, qse_lsp_obj_t* name);

qse_lsp_assoc_t* qse_lsp_insvalueintoframe (
	qse_lsp_t* lsp, qse_lsp_frame_t* frame, 
	qse_lsp_obj_t* name, qse_lsp_obj_t* value);
qse_lsp_assoc_t* qse_lsp_insfuncintoframe (
	qse_lsp_t* lsp, qse_lsp_frame_t* frame, 
	qse_lsp_obj_t* name, qse_lsp_obj_t* func);

qse_lsp_tlink_t* qse_lsp_pushtmp (qse_lsp_t* lsp, qse_lsp_obj_t* obj);
void qse_lsp_poptmp (qse_lsp_t* lsp);

#ifdef __cplusplus
}
#endif

#endif
