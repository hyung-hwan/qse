/*
 * $Id$
 *
    Copyright (c) 2006-2014 Chung, Hyung-Hwan. All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions
    are met:
    1. Redistributions of source code must retain the above copyright
       notice, this list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright
       notice, this list of conditions and the following disclaimer in the
       documentation and/or other materials provided with the distribution.

    THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
    IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
    OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
    IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
    INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
    NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
    DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
    THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
    THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _QSE_LIB_AWK_MISC_H_
#define _QSE_LIB_AWK_MISC_H_

#if defined(__cplusplus)
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

int qse_awk_buildrex (
	qse_awk_t* awk, 
	const qse_char_t* ptn,
	qse_size_t len,
	qse_awk_errnum_t* errnum,
	void** code, 
	void** icode
);

int qse_awk_matchrex (
	qse_awk_t* awk, void* code, int icase,
	const qse_cstr_t* str, const qse_cstr_t* substr,
	qse_cstr_t* match, qse_awk_errnum_t* errnum
);

void qse_awk_freerex (qse_awk_t* awk, void* code, void* icode);

int qse_awk_rtx_matchrex (
	qse_awk_rtx_t* rtx, qse_awk_val_t* val,
	const qse_cstr_t* str, const qse_cstr_t* substr,
	qse_cstr_t* match
);

#if defined(__cplusplus)
}
#endif

#endif

