/*
 * $Id: std.h 336 2010-07-24 12:43:26Z hyunghwan.chung $
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

#ifndef _QSE_AWK_STD_H_
#define _QSE_AWK_STD_H_

#include <qse/awk/awk.h>

/** @file
 * This file defines functions and data types that help you create
 * an awk interpreter with less effort. It is designed to be as close
 * to conventional awk implementations as possible.
 * 
 * The source script handler does not evaluate a file name of the "var=val"
 * form as an assignment expression. Instead, it just treats it as a
 * normal file name.
 */

/**
 * The qse_awk_parsestd_type_t type defines a source script type
 */
enum qse_awk_parsestd_type_t
{
	QSE_AWK_PARSESTD_FILE  = 0, /**< file name */
	QSE_AWK_PARSESTD_CP    = 1, /**< character pointer */
	QSE_AWK_PARSESTD_CPL   = 2, /**< character pointer + length */
	QSE_AWK_PARSESTD_STDIO = 3  /**< standard input/output */
};
typedef enum qse_awk_parsestd_type_t qse_awk_parsestd_type_t;

/**
 * The qse_awk_parsestd_in_t type defines a source input.
 */
struct qse_awk_parsestd_in_t
{
	qse_awk_parsestd_type_t type;

	union
	{
		const qse_char_t* file; 
		const qse_char_t* cp;
		qse_cstr_t        cpl;
	} u;
};
typedef struct qse_awk_parsestd_in_t qse_awk_parsestd_in_t;

/**
 * The qse_awk_parsestd_out_t type defines a source output.
 */
struct qse_awk_parsestd_out_t
{
	qse_awk_parsestd_type_t type;

	union
	{
		const qse_char_t* file; 
		qse_char_t*       cp;
		qse_xstr_t        cpl;
	} u;
};
typedef struct qse_awk_parsestd_out_t qse_awk_parsestd_out_t;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * The qse_awk_openstd() function creates an awk object.
 */
qse_awk_t* qse_awk_openstd (
	qse_size_t xtnsize /**< size of extension in bytes */
);

qse_awk_t* qse_awk_openstdwithmmgr (
	qse_mmgr_t* mmgr,
	qse_size_t  xtnsize
);

/**
 * The qse_awk_getxtnstd() gets the pointer to extension space. 
 * Note that you must not call qse_awk_getxtn() for an awk object
 * created with qse_awk_openstd().
 */
void* qse_awk_getxtnstd (
	qse_awk_t* awk
);

/**
 * The qse_awk_parsestd() functions parses source script.
 * The code below shows how to parse a literal string 'BEGIN { print 10; }' 
 * and deparses it out to a buffer 'buf'.
 * @code
 * int n;
 * qse_awk_parsestd_in_t in;
 * qse_awk_parsestd_out_t out;
 * qse_char_t buf[1000];
 *
 * qse_memset (buf, QSE_T(' '), QSE_COUNTOF(buf));
 * buf[QSE_COUNTOF(buf)-1] = QSE_T('\0');
 * in.type = QSE_AWK_PARSESTD_CP;
 * in.u.cp = QSE_T("BEGIN { print 10; }");
 * out.type = QSE_AWK_PARSESTD_CP;
 * out.u.cp = buf;
 *
 * n = qse_awk_parsestd (awk, &in, &out);
 * @endcode
 */
int qse_awk_parsestd (
	qse_awk_t*                      awk,
	const qse_awk_parsestd_in_t* in,
	qse_awk_parsestd_out_t*      out
);

/**
 * The qse_awk_rtx_openstd() function creates a standard runtime context.
 * The caller should keep the contents of @a icf and @a ocf valid throughout
 * the lifetime of the runtime context created. 
 */
qse_awk_rtx_t* qse_awk_rtx_openstd (
	qse_awk_t*             awk,
	qse_size_t             xtn,
	const qse_char_t*      id,
	const qse_char_t*const icf[],
	const qse_char_t*const ocf[]
);

/**
 * The qse_awk_rtx_getxtnstd() gets the pointer to extension space.
 */
void* qse_awk_rtx_getxtnstd (
	qse_awk_rtx_t* rtx
);

#ifdef __cplusplus
}
#endif


#endif
