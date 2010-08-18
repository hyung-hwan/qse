/*
 * $Id: mem.h 117 2008-03-03 11:20:05Z baconevi $
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

#ifndef _QSE_LIB_LSP_MEM_H_
#define _QSE_LIB_LSP_MEM_H_

#ifndef _QSE_LSP_LSP_H_
#error Never include this file directly. Include <qse/lsp/lsp.h> instead
#endif

typedef struct qse_lsp_mem_t qse_lsp_mem_t;

struct qse_lsp_mem_t
{
	qse_lsp_t* lsp;

	/* object allocation list */
	qse_size_t ubound; /* upper bounds of the maximum number of objects */
	qse_size_t ubound_inc; /* increment of the upper bounds */
	qse_size_t count;  /* the number of objects currently allocated */
	qse_lsp_obj_t* used[QSE_LSP_TYPE_COUNT];
	qse_lsp_obj_t* free[QSE_LSP_TYPE_COUNT];

	struct
	{
		qse_lsp_obj_t* obj;
		qse_lsp_obj_t* tmp; /* temporary object to protect from gc in read() */
		qse_lsp_obj_t* stack;
	} r;

	/* commonly accessed objects */
	qse_lsp_obj_t* nil;     /* qse_lsp_obj_nil_t */
	qse_lsp_obj_t* t;       /* qse_lsp_obj_true_t */
	qse_lsp_obj_t* quote;   /* qse_lsp_obj_sym_t */
	qse_lsp_obj_t* lambda;  /* qse_lsp_obj_sym_t */
	qse_lsp_obj_t* macro;   /* qse_lsp_obj_sym_t */
	qse_lsp_obj_t* num[10]; /* qse_lsp_obj_int_t */

	/* run-time environment frame */
	qse_lsp_frame_t* frame;
	/* pointer to a global-level frame */
	qse_lsp_frame_t* root_frame;
	/* pointer to an interim frame not yet added to "frame" */
	qse_lsp_frame_t* brooding_frame; 

	/* links for temporary objects */
	qse_lsp_tlink_t* tlink;
	qse_size_t tlink_count;
};

#ifdef __cplusplus
extern "C" {
#endif
	
qse_lsp_mem_t* qse_lsp_openmem (
	qse_lsp_t* lsp, qse_size_t ubound, qse_size_t ubound_inc);
void qse_lsp_closemem (qse_lsp_mem_t* mem);

/* object creation of standard types */
qse_lsp_obj_t* qse_lsp_makenil (qse_lsp_mem_t* mem);
qse_lsp_obj_t* qse_lsp_maketrue (qse_lsp_mem_t* mem);
qse_lsp_obj_t* qse_lsp_makeint (qse_lsp_mem_t* mem, qse_long_t value);
qse_lsp_obj_t* qse_lsp_makereal (qse_lsp_mem_t* mem, qse_real_t value);

qse_lsp_obj_t* qse_lsp_makesym (
	qse_lsp_mem_t* mem, const qse_char_t* str, qse_size_t len);
qse_lsp_obj_t* qse_lsp_makestr (
	qse_lsp_mem_t* mem, const qse_char_t* str, qse_size_t len);

qse_lsp_obj_t* qse_lsp_makecons (
	qse_lsp_mem_t* mem, qse_lsp_obj_t* car, qse_lsp_obj_t* cdr);
qse_lsp_obj_t* qse_lsp_makefunc (
	qse_lsp_mem_t* mem, qse_lsp_obj_t* formal, qse_lsp_obj_t* body);
qse_lsp_obj_t* qse_lsp_makemacro (
	qse_lsp_mem_t* mem, qse_lsp_obj_t* formal, qse_lsp_obj_t* body);

qse_lsp_obj_t* qse_lsp_makeprim (qse_lsp_mem_t* mem, 
	qse_lsp_prim_t impl, qse_size_t min_args, qse_size_t max_args);

/* frame lookup */
qse_lsp_assoc_t* qse_lsp_lookup (qse_lsp_mem_t* mem, qse_lsp_obj_t* name);
qse_lsp_assoc_t* qse_lsp_setvalue (
	qse_lsp_mem_t* mem, qse_lsp_obj_t* name, qse_lsp_obj_t* value);
qse_lsp_assoc_t* qse_lsp_setfunc (
	qse_lsp_mem_t* mem, qse_lsp_obj_t* name, qse_lsp_obj_t* func);

/* cons operations */
qse_size_t qse_lsp_conslen (qse_lsp_mem_t* mem, qse_lsp_obj_t* obj);

#ifdef __cplusplus
}
#endif

#endif
