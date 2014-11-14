/*
 * $Id$
 *
    Copyright 2006-2014 Chung, Hyung-Hwan.
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

#ifndef _QSE_CMN_TASK_H_
#define _QSE_CMN_TASK_H_

#include <qse/types.h>
#include <qse/macros.h>

typedef struct qse_task_t qse_task_t;
typedef struct qse_task_slice_t qse_task_slice_t;

typedef qse_task_slice_t* (*qse_task_fnc_t) (
	qse_task_t*       task,
	qse_task_slice_t* slice,
	void*             ctx
);

#if defined(__cplusplus)
extern "C" {
#endif

QSE_EXPORT qse_task_t* qse_task_open (
	qse_mmgr_t* mmgr,
	qse_size_t  xtnsize	
);

QSE_EXPORT void qse_task_close (
	qse_task_t* task
);

QSE_EXPORT qse_mmgr_t* qse_task_getmmgr (
	qse_task_t* task
);

QSE_EXPORT void* qse_task_getxtn (
	qse_task_t* task
);

QSE_EXPORT qse_task_slice_t* qse_task_create (
	qse_task_t*    task,
	qse_task_fnc_t fnc,
	void*          ctx,
	qse_size_t     stksize
);

QSE_EXPORT int qse_task_boot (
	qse_task_t*       task,
	qse_task_slice_t* to
);

QSE_EXPORT void qse_task_schedule (
	qse_task_t*       task,
	qse_task_slice_t* from,
	qse_task_slice_t* to
);


#if defined(__cplusplus)
}
#endif


#endif
