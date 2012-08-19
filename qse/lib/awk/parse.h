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

#ifndef _QSE_LIB_AWK_PARSE_H_
#define _QSE_LIB_AWK_PARSE_H_

/* these enums should match kwtab in parse.c */
enum qse_awk_kwid_t
{
	QSE_AWK_KWID_BEGIN,
	QSE_AWK_KWID_END,
	QSE_AWK_KWID_ABORT,
	QSE_AWK_KWID_BREAK,
	QSE_AWK_KWID_CONTINUE,
	QSE_AWK_KWID_DELETE,
	QSE_AWK_KWID_DO,
	QSE_AWK_KWID_ELSE,
	QSE_AWK_KWID_EXIT,
	QSE_AWK_KWID_FOR,
	QSE_AWK_KWID_FUNCTION,
	QSE_AWK_KWID_GETLINE,
	QSE_AWK_KWID_GLOBAL,
	QSE_AWK_KWID_IF,
	QSE_AWK_KWID_IN,
	QSE_AWK_KWID_INCLUDE,
	QSE_AWK_KWID_LOCAL,
	QSE_AWK_KWID_NEXT,
	QSE_AWK_KWID_NEXTFILE,
	QSE_AWK_KWID_NEXTOFILE,
	QSE_AWK_KWID_PRINT,
	QSE_AWK_KWID_PRINTF,
	QSE_AWK_KWID_RESET,
	QSE_AWK_KWID_RETURN,
	QSE_AWK_KWID_WHILE
};

typedef enum qse_awk_kwid_t qse_awk_kwid_t;

#ifdef __cplusplus
extern "C" {
#endif

int qse_awk_putsrcstr (
	qse_awk_t*        awk,
	const qse_char_t* str
);

int qse_awk_putsrcstrn (
	qse_awk_t*        awk,
	const qse_char_t* str,
	qse_size_t        len
);

const qse_char_t* qse_awk_getgblname (
	qse_awk_t*  awk,
	qse_size_t  idx,
	qse_size_t* len
);

void qse_awk_getkwname (
	qse_awk_t*     awk,
	qse_awk_kwid_t id, 
	qse_cstr_t*    s
);

int qse_awk_initgbls (
	qse_awk_t* awk
);

#ifdef __cplusplus
}
#endif

#endif
