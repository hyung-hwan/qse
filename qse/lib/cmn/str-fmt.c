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

#include <qse/cmn/str.h>
#include <qse/cmn/mbwc.h>
#include "fmt.h"

struct mbuf_t
{
	qse_mchar_t* ptr;
	qse_size_t   len;
	qse_size_t   capa;
};
typedef struct mbuf_t mbuf_t;

struct wbuf_t
{
	qse_wchar_t* ptr;
	qse_size_t   len;
	qse_size_t   capa;
};
typedef struct wbuf_t wbuf_t;


static int put_mchar_null (qse_mchar_t c, void* ctx)
{
	return 1;
}

static int put_wchar_null (qse_wchar_t c, void* ctx)
{
	return 1;
}

static int put_mchar (qse_mchar_t c, void* ctx)
{
	mbuf_t* buf = (mbuf_t*)ctx;

	if (buf->len < buf->capa) 
	{
		buf->ptr[buf->len++] = c;
		return 1;
	}

	/* buffer is full stop. but no error */
	return 0;
}

static int put_wchar (qse_wchar_t c, void* ctx)
{
	wbuf_t* buf = (wbuf_t*)ctx;

	if (buf->len < buf->capa) 
	{
		buf->ptr[buf->len++] = c;
		return 1;
	}

	/* buffer is full stop. but no error */
	return 0; 
}

static int put_mchar_strict (qse_mchar_t c, void* ctx)
{
	mbuf_t* buf = (mbuf_t*)ctx;

	if (buf->len < buf->capa) 
	{
		buf->ptr[buf->len++] = c;
		return 1;
	}

	/* buffer is full stop. return error */
	return -1;
}

static int put_wchar_strict (qse_wchar_t c, void* ctx)
{
	wbuf_t* buf = (wbuf_t*)ctx;

	if (buf->len < buf->capa) 
	{
		buf->ptr[buf->len++] = c;
		return 1;
	}

	/* buffer is full stop. return error */
	return -1; 
}


static int wcs_to_mbs (
	const qse_wchar_t* wcs, qse_size_t* wcslen,
	qse_mchar_t* mbs, qse_size_t* mbslen, void* ctx)
{
	return qse_wcsntombsnwithcmgr (wcs, wcslen, mbs, mbslen, qse_getdflcmgr());
}

static int mbs_to_wcs (
	const qse_mchar_t* mbs, qse_size_t* mbslen, 
	qse_wchar_t* wcs, qse_size_t* wcslen, void* ctx)
{
	return qse_mbsntowcsnwithcmgr (mbs, mbslen, wcs, wcslen, qse_getdflcmgr());
}

/* ----------------------------------- */

#undef T
#undef char_t
#undef buf_t
#undef fmtout_t
#undef fmtout
#undef put_char_null
#undef put_char
#undef conv_char
#undef strfmt
#undef strxfmt
#undef strvfmt
#undef strxvfmt

#define T(x) QSE_MT(x)
#define char_t qse_mchar_t
#define buf_t mbuf_t
#define fmtout_t qse_mfmtout_t
#define fmtout qse_mfmtout
#define put_char_null put_mchar_null
#define put_char put_mchar
#define conv_char wcs_to_mbs
#define strfmt qse_mbsfmt
#define strxfmt qse_mbsxfmt
#define strvfmt qse_mbsvfmt
#define strxvfmt qse_mbsxvfmt
#include "str-fmt.h"

/* ----------------------------------- */

#undef T
#undef char_t
#undef buf_t
#undef fmtout_t
#undef fmtout
#undef put_char_null
#undef put_char
#undef conv_char
#undef strfmt
#undef strxfmt
#undef strvfmt
#undef strxvfmt

#define T(x) QSE_MT(x)
#define char_t qse_mchar_t
#define buf_t mbuf_t
#define fmtout_t qse_mfmtout_t
#define fmtout qse_mfmtout
#define put_char_null put_mchar_null
#define put_char put_mchar_strict
#define conv_char wcs_to_mbs
#define strfmt qse_mbsfmts
#define strxfmt qse_mbsxfmts
#define strvfmt qse_mbsvfmts
#define strxvfmt qse_mbsxvfmts
#include "str-fmt.h"

/* ----------------------------------- */

#undef T
#undef char_t
#undef buf_t
#undef fmtout_t
#undef fmtout
#undef put_char_null
#undef put_char
#undef conv_char
#undef strfmt
#undef strxfmt
#undef strvfmt
#undef strxvfmt

#define T(x) QSE_WT(x)
#define char_t qse_wchar_t
#define buf_t wbuf_t
#define fmtout_t qse_wfmtout_t
#define fmtout qse_wfmtout
#define put_char_null put_wchar_null
#define put_char put_wchar
#define conv_char mbs_to_wcs
#define strfmt qse_wcsfmt
#define strxfmt qse_wcsxfmt
#define strvfmt qse_wcsvfmt
#define strxvfmt qse_wcsxvfmt
#include "str-fmt.h"

/* ----------------------------------- */

#undef T
#undef char_t
#undef buf_t
#undef fmtout_t
#undef fmtout
#undef put_char_null
#undef put_char
#undef conv_char
#undef strfmt
#undef strxfmt
#undef strvfmt
#undef strxvfmt

#define T(x) QSE_WT(x)
#define char_t qse_wchar_t
#define buf_t wbuf_t
#define fmtout_t qse_wfmtout_t
#define fmtout qse_wfmtout
#define put_char_null put_wchar_null
#define put_char put_wchar_strict
#define conv_char mbs_to_wcs
#define strfmt qse_wcsfmts
#define strxfmt qse_wcsxfmts
#define strvfmt qse_wcsvfmts
#define strxvfmt qse_wcsxvfmts
#include "str-fmt.h"
