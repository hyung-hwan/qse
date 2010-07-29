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

#ifndef _QSE_CMN_XMA_H_
#define _QSE_CMN_XMA_H_

#include <qse/types.h>
#include <qse/macros.h>

typedef struct qse_xma_t qse_xma_t;
typedef struct qse_xma_blk_t qse_xma_blk_t;

#define QSE_XMA_FIXED 32
#define QSE_XMA_SIZE_BITS ((QSE_SIZEOF_SIZE_T*8)-1)

struct qse_xma_t
{
	QSE_DEFINE_COMMON_FIELDS (xma)

	qse_xma_blk_t* head;
	qse_xma_blk_t* xfree[QSE_XMA_FIXED + QSE_XMA_SIZE_BITS + 1];
	qse_size_t     bdec;

#ifdef QSE_XMA_ENABLE_STAT
	struct
	{
		qse_size_t total;
		qse_size_t alloc;
		qse_size_t avail;
		qse_size_t nused;
		qse_size_t nfree;
	} stat;
#endif
};

#ifdef __cplusplus
extern "C" {
#endif

QSE_DEFINE_COMMON_FUNCTIONS (xma)

qse_xma_t* qse_xma_open (
	qse_mmgr_t* mmgr,
	qse_size_t  ext,
	qse_size_t  size
);

void qse_xma_close (
	qse_xma_t* xma
);

qse_xma_t* qse_xma_init (
	qse_xma_t*  xma,
	qse_mmgr_t* mmgr,
	qse_size_t  size
);

void qse_xma_fini (
	qse_xma_t* xma
);

void* qse_xma_alloc (
	qse_xma_t* xma,
	qse_size_t size
);

void* qse_xma_realloc (
	qse_xma_t* xma, 
	void*      b,
	qse_size_t size
);

void qse_xma_free (
	qse_xma_t* xma,
	void*      b
);

#ifdef __cplusplus
}
#endif

#endif
