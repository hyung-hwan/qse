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


static int put_mchar (qse_mchar_t c, void* ctx)
{
	mbuf_t* buf = (mbuf_t*)ctx;

	/* do not copy but return success if the buffer pointer 
	 * points to NULL. this is to let the caller specify
	 * NULL as a buffer to get the length required for the
	 * full formatting excluding the terminating NULL. 
	 * The actual length required is the return value + 1. */
	if (buf->ptr == QSE_NULL) 
	{
		buf->len++;
		return 1;
	}
	else if (buf->len < buf->capa) 
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

	/* do not copy but return success if the buffer pointer 
	 * points to NULL. this is to let the caller specify
	 * NULL as a buffer to get the length required for the
	 * full formatting excluding the terminating NULL. 
	 * The actual length required is the return value + 1. */
	if (buf->ptr == QSE_NULL) return 1;

	if (buf->len < buf->capa) 
	{
		buf->ptr[buf->len++] = c;
		return 1;
	}

	/* buffer is full stop. but no error */
	return 0; 
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
#undef put_char
#undef conv_char
#undef strfmt
#undef strxfmt

#define T(x) QSE_MT(x)
#define char_t qse_mchar_t
#define buf_t mbuf_t
#define fmtout_t qse_mfmtout_t
#define fmtout qse_mfmtout
#define put_char put_mchar
#define conv_char wcs_to_mbs
#define strfmt qse_mbsfmt
#define strxfmt qse_mbsxfmt
#include "str-fmt.h"

/* ----------------------------------- */

#undef T
#undef char_t
#undef buf_t
#undef fmtout_t
#undef fmtout
#undef put_char
#undef conv_char
#undef strfmt
#undef strxfmt

#define T(x) QSE_WT(x)
#define char_t qse_wchar_t
#define buf_t wbuf_t
#define fmtout_t qse_wfmtout_t
#define fmtout qse_wfmtout
#define put_char put_wchar
#define conv_char mbs_to_wcs
#define strfmt qse_wcsfmt
#define strxfmt qse_wcsxfmt
#include "str-fmt.h"

