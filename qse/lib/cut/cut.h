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

#ifndef _QSE_LIB_CUT_CUT_H_
#define _QSE_LIB_CUT_CUT_H_

#include <qse/cut/cut.h>
#include <qse/cmn/str.h>

typedef struct qse_cut_sel_blk_t qse_cut_sel_blk_t;

struct qse_cut_sel_blk_t
{
	qse_size_t len;
	struct
	{
		enum
		{
			QSE_SED_SEL_CHAR = QSE_T('c'),
			QSE_SED_SEL_FIELD = QSE_T('f')
		} id;
		qse_size_t start;
		qse_size_t end;
	} range[128];
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
		qse_cut_sel_blk_t  fb; /**< the first block is static */
		qse_cut_sel_blk_t* lb; /**< points to the last block */

		qse_char_t         din; /**< input field delimiter */
		qse_char_t         dout; /**< output field delimiter */

		qse_size_t         count; 
		qse_size_t         fcount; 
		qse_size_t         ccount; 
        } sel;

	struct
	{
		/** data needed for output streams */
		struct
		{
			qse_cut_io_fun_t fun; /**< an output handler */
			qse_cut_io_arg_t arg; /**< output handling data */

			qse_char_t buf[2048];
			qse_size_t len;
			int        eof;
		} out;
		
		/** data needed for input streams */
		struct
		{
			qse_cut_io_fun_t fun; /**< an input handler */
			qse_cut_io_arg_t arg; /**< input handling data */

			qse_char_t xbuf[1]; /**< a read-ahead buffer */
			int xbuf_len; /**< data length in the buffer */

			qse_char_t buf[2048]; /**< input buffer */
			qse_size_t len; /**< data length in the buffer */
			qse_size_t pos; /**< current position in the buffer */
			int        eof; /**< EOF indicator */

			qse_str_t line; /**< pattern space */
			qse_size_t num; /**< current line number */

			qse_size_t  nflds; /**< the number of fields */
			qse_size_t  cflds; /**< capacity of flds field */
			qse_cstr_t  sflds[128]; /**< static field buffer */
			qse_cstr_t* flds;
			int delimited;
		} in;
	} e;
};

#ifdef __cplusplus
extern "C" {
#endif

const qse_char_t* qse_cut_dflerrstr (qse_cut_t* cut, qse_cut_errnum_t errnum);

#ifdef __cplusplus
}
#endif

#endif
