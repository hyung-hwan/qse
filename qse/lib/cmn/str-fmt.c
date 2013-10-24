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
#include "fmt.h"

struct wbuf_t
{
	qse_wchar_t* ptr;
	qse_size_t   len;
	qse_size_t   capa;
};

struct mbuf_t
{
	qse_mchar_t* ptr;
	qse_size_t   len;
	qse_size_t   capa;
};

typedef struct wbuf_t wbuf_t;
typedef struct mbuf_t mbuf_t;

static int put_wchar_to_wbuf (qse_wchar_t c, void* arg)
{
	qse_fmtout_t* fo = (qse_fmtout_t*)arg;
	wbuf_t* buf = (wbuf_t*)fo->ctx;
	if (buf->len < buf->capa) buf->ptr[buf->len++] = c;
	return 1;
}

static int put_mchar_to_mbuf (qse_mchar_t c, void* arg)
{
	qse_fmtout_t* fo = (qse_fmtout_t*)arg;
	mbuf_t* buf = (mbuf_t*)fo->ctx;
	if (buf->len < buf->capa) buf->ptr[buf->len++] = c;
	return 1;
}

static int put_wchar_to_mbuf (qse_wchar_t c, void* arg)
{
	return 1;
}

static int put_mchar_to_wbuf (qse_mchar_t c, void* arg)
{
	return 1;
}

/* ----------------------------------- */

#undef T
#undef char_t
#undef buf_t
#undef fmtout
#undef output_char
#undef output_ochar
#undef strfmt
#undef strxfmt

#define T(x) QSE_MT(x)
#define char_t qse_mchar_t
#define buf_t mbuf_t
#define fmtout qse_mfmtout
#define output_mchar put_mchar_to_mbuf
#define output_wchar put_wchar_to_mbuf
#define strfmt qse_mbsfmt
#define strxfmt qse_mbsxfmt
#include "str-fmt.h"

/* ----------------------------------- */

#undef T
#undef char_t
#undef buf_t
#undef fmtout
#undef output_mchar
#undef output_wchar
#undef strfmt
#undef strxfmt

#define T(x) QSE_WT(x)
#define char_t qse_wchar_t
#define buf_t wbuf_t
#define fmtout qse_wfmtout
#define output_mchar put_mchar_to_wbuf
#define output_wchar put_wchar_to_wbuf
#define strfmtx qse_wcsfmt
#define strxfmtx qse_wcsxfmt
#include "str-fmt.h"


/* ----------------------------------- */

#undef T
#undef char_t
#undef buf_t
#undef fmtout
#undef output_mchar
#undef output_wchar
#undef strfmt
#undef strxfmt

#define T(x) QSE_T(x)
#define char_t qse_char_t
#if defined(QSE_CHAR_IS_MCHAR)
#	define buf_t mbuf_t
#	define output_mchar put_mchar_to_mbuf
#	define output_wchar put_wchar_to_mbuf
#	define fmtout qse_mfmtout
#else
#	define buf_t wbuf_t
#	define output_mchar put_mchar_to_wbuf
#	define output_wchar put_wchar_to_wbuf
#	define fmtout qse_wfmtout
#endif
#define strfmt qse_strfmt
#define strxfmt qse_strxfmt
#include "str-fmt.h"
