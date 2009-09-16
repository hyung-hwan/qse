/*
 * $Id: parse.h 287 2009-09-15 10:01:02Z hyunghwan.chung $
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

#ifndef _QSE_LIB_AWK_PARSE_H_
#define _QSE_LIB_AWK_PARSE_H_

/* these enums should match kwtab in parse.c */
enum kw_t
{
	KW_BEGIN,
	KW_END,
	KW_BREAK,
	KW_CONTINUE,
	KW_DELETE,
	KW_DO,
	KW_ELSE,
	KW_EXIT,
	KW_FOR,
	KW_FUNCTION,
	KW_GETLINE,
	KW_GLOBAL,
	KW_IF,
	KW_IN,
	KW_INCLUDE,
	KW_LOCAL,
	KW_NEXT,
	KW_NEXTFILE,
	KW_NEXTOFILE,
	KW_PRINT,
	KW_PRINTF,
	KW_RESET,
	KW_RETURN,
	KW_WHILE
};

#ifdef __cplusplus
extern "C" {
#endif

int qse_awk_putsrcstr (qse_awk_t* awk, const qse_char_t* str);
int qse_awk_putsrcstrx (
	qse_awk_t* awk, const qse_char_t* str, qse_size_t len);

const qse_char_t* qse_awk_getgblname (
	qse_awk_t* awk, qse_size_t idx, qse_size_t* len);
qse_cstr_t* qse_awk_getkw (qse_awk_t* awk, int id, qse_cstr_t* s);

int qse_awk_initgbls (qse_awk_t* awk);

#ifdef __cplusplus
}
#endif

#endif
