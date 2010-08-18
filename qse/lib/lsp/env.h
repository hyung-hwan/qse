/*
 * $Id: env.h 117 2008-03-03 11:20:05Z baconevi $
 *
    Copyright 2006-2009 Chung, Hyung-Hwan.
    This file is part of QSE.

    QSE is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as 
    published by the Free Software Foundation, either version 3 of 
    the License, or (at your option) any later version.

    QSE is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public 
    License along with QSE. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _QSE_LIB_LSP_ENV_H_
#define _QSE_LIB_LSP_ENV_H_

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
