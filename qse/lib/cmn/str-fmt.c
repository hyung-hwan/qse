/*
 * $Id$
 *
    Copyright 2006-2014 Chung, Hyung-Hwan.
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
