/*
 * $Id: misc.h 515 2011-07-22 15:43:03Z hyunghwan.chung $
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

#ifndef _QSE_LIB_AWK_MISC_H_
#define _QSE_LIB_AWK_MISC_H_

#ifdef __cplusplus
extern "C" {
#endif

qse_char_t* qse_awk_rtx_strtok (
	qse_awk_rtx_t* rtx, const qse_char_t* s, 
	const qse_char_t* delim, qse_cstr_t* tok);

qse_char_t* qse_awk_rtx_strxtok (
	qse_awk_rtx_t* rtx, const qse_char_t* s, qse_size_t len,
	const qse_char_t* delim, qse_cstr_t* tok);

qse_char_t* qse_awk_rtx_strntok (
	qse_awk_rtx_t* rtx, const qse_char_t* s, 
	const qse_char_t* delim, qse_size_t delim_len, qse_cstr_t* tok);

qse_char_t* qse_awk_rtx_strxntok (
	qse_awk_rtx_t* rtx, const qse_char_t* s, qse_size_t len,
	const qse_char_t* delim, qse_size_t delim_len, qse_cstr_t* tok);

qse_char_t* qse_awk_rtx_strxntokbyrex (
	qse_awk_rtx_t*    rtx, 
	const qse_char_t* str,
	qse_size_t        len,
	const qse_char_t* substr,
	qse_size_t        sublen,
	void*             rex,
	qse_cstr_t*       tok,
	qse_awk_errnum_t* errnum
);

qse_char_t* qse_awk_rtx_strxnfld (
	qse_awk_rtx_t* rtx,
	qse_char_t*    str,
	qse_size_t     len,
	qse_char_t     fs,
	qse_char_t     lq,
	qse_char_t     rq,
	qse_char_t     ec,
	qse_cstr_t*    tok
);

void* qse_awk_buildrex (
	qse_awk_t* awk, 
	const qse_char_t* ptn,
	qse_size_t len,
	qse_awk_errnum_t* errnum
);

int qse_awk_matchrex (
	qse_awk_t* awk, void* code, int option,
	const qse_cstr_t* str, const qse_cstr_t* substr,
	qse_cstr_t* match, qse_awk_errnum_t* errnum
);

int qse_awk_sprintflt (
	qse_awk_t*  awk,
	qse_char_t* buf,
	qse_size_t  len,
	qse_flt_t   num
);


#ifdef __cplusplus
}
#endif

#endif

