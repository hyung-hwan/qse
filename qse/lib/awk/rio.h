/*
 * $Id$
 *
    Copyright (c) 2006-2019 Chung, Hyung-Hwan. All rights reserved.

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

#ifndef _QSE_LIB_AWK_RIO_H_
#define _QSE_LIB_AWK_RIO_H_

#if defined(__cplusplus)
extern "C" {
#endif

int qse_awk_rtx_readio (
	qse_awk_rtx_t* run, int in_type, 
	const qse_char_t* name, qse_str_t* buf);

int qse_awk_rtx_writeioval (
	qse_awk_rtx_t* run, int out_type, 
	const qse_char_t* name, qse_awk_val_t* v);

int qse_awk_rtx_writeiostr (
	qse_awk_rtx_t* run, int out_type, 
	const qse_char_t* name, qse_char_t* str, qse_size_t len);

int qse_awk_rtx_writeiobytes (
	qse_awk_rtx_t* run, int out_type, 
	const qse_char_t* name, qse_mchar_t* str, qse_size_t len);

int qse_awk_rtx_flushio (
	qse_awk_rtx_t* run, int out_type, const qse_char_t* name);

int qse_awk_rtx_nextio_read (
	qse_awk_rtx_t* run, int in_type, const qse_char_t* name);

int qse_awk_rtx_nextio_write (
	qse_awk_rtx_t* run, int out_type, const qse_char_t* name);

int qse_awk_rtx_closeio (
	qse_awk_rtx_t*    run,
	const qse_char_t* name,
	const qse_char_t* opt
);

void qse_awk_rtx_cleario (qse_awk_rtx_t* run);

#if defined(__cplusplus)
}
#endif

#endif
