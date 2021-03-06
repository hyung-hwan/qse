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

#include <qse/cmn/str.h>
#include <qse/cmn/mbwc.h>
#include "mem-prv.h"
#include "va_copy.h"

static int put_mchar_null (qse_mchar_t c, void* ctx)
{
	return 1;
}

static int put_wchar_null (qse_wchar_t c, void* ctx)
{
	return 1;
}

static int put_mchar_check (qse_mchar_t c, void* ctx)
{
	qse_mbs_t* str = (qse_mbs_t*)ctx;
	if (str->val.len < str->capa) str->val.ptr[str->val.len++] = c;
	return 1;
}

static int put_wchar_check (qse_wchar_t c, void* ctx)
{
	qse_wcs_t* str = (qse_wcs_t*)ctx;
	if (str->val.len < str->capa) str->val.ptr[str->val.len++] = c;
	return 1; 
}

static int put_mchar_nocheck (qse_mchar_t c, void* ctx)
{
	qse_mbs_t* str = (qse_mbs_t*)ctx;
	str->val.ptr[str->val.len++] = c;
	return 1;
}

static int put_wchar_nocheck (qse_wchar_t c, void* ctx)
{
	qse_wcs_t* str = (qse_wcs_t*)ctx;
	str->val.ptr[str->val.len++] = c;
	return 1; 
}

static int wcs_to_mbs (
	const qse_wchar_t* wcs, qse_size_t* wcslen,
	qse_mchar_t* mbs, qse_size_t* mbslen, void* ctx)
{
	return qse_wcsntombsnwithcmgr(wcs, wcslen, mbs, mbslen, qse_getdflcmgr());
}

static int mbs_to_wcs (
	const qse_mchar_t* mbs, qse_size_t* mbslen, 
	qse_wchar_t* wcs, qse_size_t* wcslen, void* ctx)
{
	return qse_mbsntowcsnwithcmgr(mbs, mbslen, wcs, wcslen, qse_getdflcmgr());
}

/* -------------------------------------------------------- */

#undef char_t
#undef cstr_t
#undef str_sizer_t
#undef T
#undef STR_LEN
#undef strlen
#undef strncpy
#undef strxpac
#undef strxtrm
#undef fmtout_t
#undef fmtout
#undef put_char_null
#undef put_char_check
#undef put_char_nocheck
#undef conv_char
#undef str_t
#undef str_open
#undef str_close
#undef str_init
#undef str_fini
#undef str_getmmgr
#undef str_getxtn
#undef str_yield
#undef str_yieldptr
#undef str_getsizer
#undef str_setsizer
#undef str_getcapa
#undef str_setcapa
#undef str_getlen
#undef str_setlen
#undef str_clear
#undef str_swap
#undef str_cpy
#undef str_ncpy
#undef str_cat
#undef resize_for_ncat
#undef str_ncat
#undef str_nrcat
#undef str_ccat
#undef str_nccat
#undef str_del
#undef str_amend
#undef str_trm
#undef str_pac
#undef str_fmt
#undef str_vfmt
#undef str_fcat
#undef str_vfcat

#define char_t qse_mchar_t
#define cstr_t qse_mcstr_t
#define str_sizer_t qse_mbs_sizer_t
#define T(x) QSE_MT(x)
#define STR_LEN(x) QSE_MBS_LEN(x)
#define strlen(x) qse_mbslen(x)
#define strncpy(x,y,z) qse_mbsncpy(x,y,z)
#define strxpac(x,y) qse_mbsxpac(x,y)
#define strxtrm(x,y) qse_mbsxtrm(x,y)
#define fmtout_t qse_mfmtout_t
#define fmtout qse_mfmtout
#define put_char_null put_mchar_null
#define put_char_check put_mchar_check
#define put_char_nocheck put_mchar_nocheck
#define conv_char wcs_to_mbs
#define str_t qse_mbs_t
#define str_open qse_mbs_open 
#define str_close qse_mbs_close 
#define str_init qse_mbs_init 
#define str_fini qse_mbs_fini 
#define str_getmmgr qse_mbs_getmmgr 
#define str_getxtn qse_mbs_getxtn 
#define str_yield qse_mbs_yield 
#define str_yieldptr qse_mbs_yieldptr 
#define str_getsizer qse_mbs_getsizer 
#define str_setsizer qse_mbs_setsizer 
#define str_getcapa qse_mbs_getcapa 
#define str_setcapa qse_mbs_setcapa 
#define str_getlen qse_mbs_getlen 
#define str_setlen qse_mbs_setlen 
#define str_clear qse_mbs_clear 
#define str_swap qse_mbs_swap
#define str_cpy qse_mbs_cpy
#define str_ncpy qse_mbs_ncpy
#define str_cat qse_mbs_cat
#define resize_for_ncat resize_for_mbs_ncat
#define str_ncat qse_mbs_ncat
#define str_nrcat qse_mbs_nrcat
#define str_ccat qse_mbs_ccat
#define str_nccat qse_mbs_nccat
#define str_del qse_mbs_del
#define str_amend qse_mbs_amend
#define str_trm qse_mbs_trm
#define str_pac qse_mbs_pac
#define str_fmt qse_mbs_fmt
#define str_vfmt qse_mbs_vfmt
#define str_fcat qse_mbs_fcat
#define str_vfcat qse_mbs_vfcat
#include "str-dyn.h"

/* -------------------------------------------------------- */

#undef char_t
#undef cstr_t
#undef str_sizer_t
#undef T
#undef STR_LEN
#undef strlen
#undef strncpy
#undef strxpac
#undef strxtrm
#undef fmtout_t
#undef fmtout
#undef put_char_null
#undef put_char_check
#undef put_char_nocheck
#undef conv_char
#undef str_t
#undef str_open
#undef str_close
#undef str_init
#undef str_fini
#undef str_getmmgr
#undef str_getxtn
#undef str_yield
#undef str_yieldptr
#undef str_getsizer
#undef str_setsizer
#undef str_getcapa
#undef str_setcapa
#undef str_getlen
#undef str_setlen
#undef str_clear
#undef str_swap
#undef str_cpy
#undef str_ncpy
#undef str_cat
#undef resize_for_ncat
#undef str_ncat
#undef str_nrcat
#undef str_ccat
#undef str_nccat
#undef str_del
#undef str_amend
#undef str_trm
#undef str_pac
#undef str_fmt
#undef str_vfmt
#undef str_fcat
#undef str_vfcat

#define char_t qse_wchar_t
#define cstr_t qse_wcstr_t
#define str_sizer_t qse_wcs_sizer_t
#define T(x) QSE_WT(x)
#define STR_LEN(x) QSE_WCS_LEN(x)
#define strlen(x) qse_wcslen(x)
#define strncpy(x,y,z) qse_wcsncpy(x,y,z)
#define strxpac(x,y) qse_wcsxpac(x,y)
#define strxtrm(x,y) qse_wcsxtrm(x,y)
#define fmtout_t qse_wfmtout_t
#define fmtout qse_wfmtout
#define put_char_null put_wchar_null
#define put_char_check put_wchar_check
#define put_char_nocheck put_wchar_nocheck
#define conv_char mbs_to_wcs
#define str_t qse_wcs_t
#define str_open qse_wcs_open 
#define str_close qse_wcs_close 
#define str_init qse_wcs_init 
#define str_fini qse_wcs_fini 
#define str_getmmgr qse_wcs_getmmgr 
#define str_getxtn qse_wcs_getxtn 
#define str_yield qse_wcs_yield 
#define str_yieldptr qse_wcs_yieldptr 
#define str_getsizer qse_wcs_getsizer 
#define str_setsizer qse_wcs_setsizer 
#define str_getcapa qse_wcs_getcapa 
#define str_setcapa qse_wcs_setcapa 
#define str_getlen qse_wcs_getlen 
#define str_setlen qse_wcs_setlen 
#define str_clear qse_wcs_clear 
#define str_swap qse_wcs_swap 
#define str_cpy qse_wcs_cpy 
#define str_ncpy qse_wcs_ncpy 
#define str_cat qse_wcs_cat 
#define resize_for_ncat resize_for_wcs_ncat 
#define str_ncat qse_wcs_ncat 
#define str_nrcat qse_wcs_nrcat 
#define str_ccat qse_wcs_ccat 
#define str_nccat qse_wcs_nccat 
#define str_del qse_wcs_del 
#define str_amend qse_wcs_amend 
#define str_trm qse_wcs_trm 
#define str_pac qse_wcs_pac 
#define str_fmt qse_wcs_fmt 
#define str_vfmt qse_wcs_vfmt 
#define str_fcat qse_wcs_fcat 
#define str_vfcat qse_wcs_vfcat 
#include "str-dyn.h"


qse_size_t qse_mbs_ncatwcs (qse_mbs_t* str, const qse_wchar_t* s, qse_size_t len, qse_cmgr_t* cmgr)
{
	qse_size_t mbslen, wcslen;

	wcslen = len;
	if (qse_wcsntombsnwithcmgr(s, &wcslen, QSE_NULL, &mbslen, cmgr) <= -1) return (qse_size_t)-1;

	if (resize_for_mbs_ncat(str, mbslen) <= 0) return -1;

	wcslen = len;
	mbslen = str->capa - str->val.len;
	qse_wcsntombsnwithcmgr(s, &wcslen, &str->val.ptr[str->val.len], &mbslen, cmgr);
	str->val.len += mbslen;
	str->val.ptr[str->val.len] = QSE_MT('\0');

	return str->val.len;
}

qse_size_t qse_wcs_ncatmbs (qse_wcs_t* str, const qse_mchar_t* s, qse_size_t len, qse_cmgr_t* cmgr)
{
	qse_size_t mbslen, wcslen;

	mbslen = len;
	if (qse_mbsntowcsnallwithcmgr(s, &mbslen, QSE_NULL, &wcslen, cmgr) <= -1) return (qse_size_t)-1;

	if (resize_for_wcs_ncat(str, wcslen) <= 0) return -1;

	mbslen = len;
	wcslen = str->capa - str->val.len;
	qse_mbsntowcsnallwithcmgr(s, &mbslen, &str->val.ptr[str->val.len], &wcslen, cmgr);
	str->val.len += wcslen;
	str->val.ptr[str->val.len] = QSE_WT('\0');

	return str->val.len;
}
