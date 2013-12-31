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

#ifndef _QSE_LIB_AWK_VAL_H_
#define _QSE_LIB_AWK_VAL_H_

#define QSE_AWK_VAL_CHUNK_SIZE 100

typedef struct qse_awk_val_chunk_t qse_awk_val_chunk_t;
typedef struct qse_awk_val_ichunk_t qse_awk_val_ichunk_t;
typedef struct qse_awk_val_rchunk_t qse_awk_val_rchunk_t;

struct qse_awk_val_chunk_t
{
        qse_awk_val_chunk_t* next;
};

struct qse_awk_val_ichunk_t
{
	qse_awk_val_chunk_t* next;
	/* make sure that it has the same fields as 
	   qse_awk_val_chunk_t up to this point */

	qse_awk_val_int_t slot[QSE_AWK_VAL_CHUNK_SIZE];
};

struct qse_awk_val_rchunk_t
{
	qse_awk_val_chunk_t* next;
	/* make sure that it has the same fields as 
	   qse_awk_val_chunk_t up to this point */

	qse_awk_val_flt_t slot[QSE_AWK_VAL_CHUNK_SIZE];
};

#ifdef __cplusplus
extern "C" {
#endif

/* represents a nil value */
extern qse_awk_val_t* qse_awk_val_nil;

/* represents an empty string  */
extern qse_awk_val_t* qse_awk_val_zls;

/* represents a numeric value -1 */
extern qse_awk_val_t* qse_awk_val_negone;

/* represents a numeric value 0 */
extern qse_awk_val_t* qse_awk_val_zero;

/* represents a numeric value 1 */
extern qse_awk_val_t* qse_awk_val_one;


void qse_awk_rtx_freeval (
        qse_awk_rtx_t* rtx,
        qse_awk_val_t* val,
        int            cache
);

void qse_awk_rtx_freevalchunk (
	qse_awk_rtx_t*       rtx,
	qse_awk_val_chunk_t* chunk
);

#ifdef __cplusplus
}
#endif

#endif
