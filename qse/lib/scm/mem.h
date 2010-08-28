/*
 * $Id$
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

#ifndef _QSE_LIB_SCM_MEM_H_
#define _QSE_LIB_SCM_MEM_H_

#ifndef _QSE_SCM_SCM_H_
#error Never include this file directly. Include <qse/scm/scm.h> instead
#endif

enum
{
	QSE_SCM_OBJ_NIL = 0,
	QSE_SCM_OBJ_TRUE,
	QSE_SCM_OBJ_INT,
	QSE_SCM_OBJ_REAL,
	QSE_SCM_OBJ_SYM,
	QSE_SCM_OBJ_STR,
	QSE_SCM_OBJ_CONS,
	QSE_SCM_OBJ_FUNC,
	QSE_SCM_OBJ_MACRO,
	QSE_SCM_OBJ_PRIM,

	QSE_SCM_TYPE_COUNT /* the number of lsp object types */
};


typedef struct qse_scm_mem_t qse_scm_mem_t;

struct qse_scm_mem_t
{
	qse_scm_t* scm;

	/* object allocation list */
	qse_size_t ubound; /* upper bounds of the maximum number of objects */
	qse_size_t ubound_inc; /* increment of the upper bounds */
	qse_size_t count;  /* the number of objects currently allocated */
	qse_scm_obj_t* used[QSE_SCM_TYPE_COUNT];
	qse_scm_obj_t* free[QSE_SCM_TYPE_COUNT];

	struct
	{
		qse_scm_obj_t* obj;
		qse_scm_obj_t* tmp; /* temporary datum to protect from gc in read() */
		qse_scm_obj_t* stack;
	} r;

	/* commonly accessed objects */
	qse_scm_obj_t* nil;     /* qse_scm_obj_nil_t */
	qse_scm_obj_t* t;       /* qse_scm_obj_true_t */
	qse_scm_obj_t* quote;   /* qse_scm_obj_sym_t */
	qse_scm_obj_t* lambda;  /* qse_scm_obj_sym_t */
	qse_scm_obj_t* macro;   /* qse_scm_obj_sym_t */
	qse_scm_obj_t* num[10]; /* qse_scm_obj_int_t */

#if 0
	/* run-time environment frame */
	qse_scm_frame_t* frame;
	/* pointer to a global-level frame */
	qse_scm_frame_t* root_frame;
	/* pointer to an interim frame not yet added to "frame" */
	qse_scm_frame_t* brooding_frame; 

	/* links for temporary objects */
	qse_scm_tlink_t* tlink;
	qse_size_t tlink_count;
#endif
};

#ifdef __cplusplus
extern "C" {
#endif
	
qse_scm_mem_t* qse_scm_initmem (
	qse_scm_mem_t* mem, qse_scm_t* scm,
	qse_size_t ubound, qse_size_t ubound_inc);
void qse_scm_finimem (qse_scm_mem_t* mem);

#ifdef __cplusplus
}
#endif

#endif
