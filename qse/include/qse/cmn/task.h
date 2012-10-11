/*
 * $Id$
 *
    Copyright 2006-2012 Chung, Hyung-Hwan.
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

typedef void (*qse_task_fnc_t) (void* ctx);

typedef struct qse_task_t qse_task_t;

#ifdef __cplusplus
extern "C" {
#endif

int qse_gettaskid (
	qse_task_t* task
);

qse_task_t* qse_maketask (
	qse_task_fnc_t fnc,
	void*          ctx,
	qse_size_t     stsize
);

int qse_task_boot (
	void
);

#ifdef __cplusplus
}
#endif


#endif
