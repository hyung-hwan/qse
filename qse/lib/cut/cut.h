/*
 * $Id: cut.h 287 2009-09-15 10:01:02Z baconevi $
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

#ifndef _QSE_LIB_CUT_CUT_H_
#define _QSE_LIB_CUT_CUT_H_

#include <qse/cut/cut.h>
#include <qse/cmn/str.h>

typedef struct qse_cut_sel_blk_t qse_cut_sel_blk_t;

struct qse_cut_sel_blk_t
{
	qse_size_t         len;
	struct
	{
		enum
		{
			CHAR,
			FIELD
		} type;
		qse_size_t start;
		qse_size_t end;
	} range[256];
	qse_cut_sel_blk_t* next;
};

struct qse_cut_t
{
	QSE_DEFINE_COMMON_FIELDS (cut)

	qse_cut_errstr_t errstr; /**< error string getter */
	qse_cut_errnum_t errnum; /**< stores an error number */
	qse_char_t errmsg[128];  /**< error message holder */

	int option;              /**< stores options */

	struct
	{
		struct
		{
			qse_size_t build;
			qse_size_t match; 
		} rex;
	} depth;

	struct
	{
		qse_cut_sel_blk_t  fb; /**< the first block is static */
		qse_cut_sel_blk_t* lb; /**< points to the last block */
		qse_size_t         count; 
        } sel;
};

#ifdef __cplusplus
extern "C" {
#endif

const qse_char_t* qse_cut_dflerrstr (qse_cut_t* cut, qse_cut_errnum_t errnum);

#ifdef __cplusplus
}
#endif

#endif
