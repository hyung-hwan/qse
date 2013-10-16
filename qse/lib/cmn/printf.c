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


#include <qse/cmn/chr.h>
#include <qse/cmn/str.h>
#include <qse/cmn/mbwc.h>
#include <stdarg.h>
#include "mem.h"

/* number of bits in a byte */
#define NBBY    8               

/* Max number conversion buffer length: 
 * qse_intmax_t in base 2, plus NUL byte. */
#define MAXNBUF (QSE_SIZEOF(qse_intmax_t) * NBBY + 1)

enum
{
	/* integer */
	LF_C = (1 << 0),
	LF_H = (1 << 1),
	LF_J = (1 << 2),
	LF_L = (1 << 3),
	LF_Q = (1 << 4),
	LF_T = (1 << 5),
	LF_Z = (1 << 6),

	/* long double */
	LF_LD = (1 << 7)
};

static struct
{
	qse_uint8_t flag; /* for single occurrence */
	qse_uint8_t dflag; /* for double occurrence */
} lm_tab[26] = 
{
	{ 0,    0 }, /* a */
	{ 0,    0 }, /* b */
	{ 0,    0 }, /* c */
	{ 0,    0 }, /* d */
	{ 0,    0 }, /* e */
	{ 0,    0 }, /* f */
	{ 0,    0 }, /* g */
	{ LF_H, LF_C }, /* h */
	{ 0,    0 }, /* i */
	{ LF_J, 0 }, /* j */
	{ 0,    0 }, /* k */
	{ LF_L, LF_Q }, /* l */
	{ 0,    0 }, /* m */
	{ 0,    0 }, /* n */
	{ 0,    0 }, /* o */
	{ 0,    0 }, /* p */
	{ LF_Q, 0 }, /* q */
	{ 0,    0 }, /* r */
	{ 0,    0 }, /* s */
	{ LF_T, 0 }, /* t */
	{ 0,    0 }, /* u */
	{ 0,    0 }, /* v */
	{ 0,    0 }, /* w */
	{ 0,    0 }, /* z */
	{ 0,    0 }, /* y */
	{ LF_Z, 0 }, /* z */
};


enum 
{
	FLAGC_DOT       = (1 << 0),
	FLAGC_SHARP     = (1 << 1),
	FLAGC_SIGN      = (1 << 2),
	FLAGC_SPACE     = (1 << 3),
	FLAGC_LEFTADJ   = (1 << 4),
	FLAGC_ZEROPAD   = (1 << 5),
	FLAGC_WIDTH     = (1 << 6),
	FLAGC_PRECISION = (1 << 7),
	FLAGC_STAR1     = (1 << 8),
	FLAGC_STAR2     = (1 << 9),
	FLAGC_LENMOD    = (1 << 10) /* length modifier */
};

#include <stdio.h> /* TODO: remove dependency on this */
#if defined(_MSC_VER) || defined(__BORLANDC__) || (defined(__WATCOMC__) && (__WATCOMC__ < 1200))
#	define snprintf _snprintf
#	define vsnprintf _vsnprintf
#endif

static int put_wchar (qse_wchar_t c, void *arg)
{
	qse_cmgr_t* cmgr;
	qse_mchar_t mbsbuf[QSE_MBLEN_MAX + 1];
	qse_size_t n;

	cmgr = qse_getdflcmgr ();
	n = cmgr->wctomb (c, mbsbuf, QSE_COUNTOF(mbsbuf));
	if (n <= 0 || n > QSE_COUNTOF(mbsbuf))
	{
		return (putchar ('?') == EOF)? -1: 0;
	}
	else
	{
		qse_size_t i;
		for (i = 0; i < n; i++) 
		{
			if (putchar (mbsbuf[i]) == EOF) return -1;
		}
		return 0;
	}
}

static int put_mchar (qse_mchar_t c, void *arg)
{
	return (putchar (c) == EOF)? -1: 0;
}

#undef char_t
#undef uchar_t
#undef ochar_t
#undef T
#undef OT
#undef toupper
#undef hex2ascii
#undef sprintn
#undef xprintf

#define char_t qse_mchar_t
#define uchar_t qse_mchar_t
#define ochar_t qse_wchar_t
#define T(x) QSE_MT(x)
#define OT(x) QSE_WT(x)
#define toupper QSE_TOUPPER
#define sprintn m_sprintn
#define xprintf qse_mxprintf 

static const qse_mchar_t m_hex2ascii[] = QSE_MT("0123456789abcdefghijklmnopqrstuvwxyz");
#define hex2ascii(hex)  (m_hex2ascii[hex])

#include "printf.h"

int qse_mprintf (const char_t *fmt, ...)
{
	va_list ap;
	int n;
	va_start (ap, fmt);
	n = qse_mxprintf (fmt, put_mchar, put_wchar, QSE_NULL, ap);
	va_end (ap);
	return n;
}

int qse_mvprintf (const char_t* fmt, va_list ap)
{
	return qse_mxprintf (fmt, put_mchar, put_wchar, QSE_NULL, ap);
}

/* ------------------------------------------------------------------ */

#undef char_t
#undef uchar_t
#undef ochar_t
#undef T
#undef OT
#undef toupper
#undef hex2ascii
#undef sprintn
#undef xprintf

#define char_t qse_wchar_t
#define uchar_t qse_wchar_t
#define ochar_t qse_mchar_t
#define T(x) QSE_WT(x)
#define OT(x) QSE_MT(x)
#define toupper QSE_TOWUPPER
#define sprintn w_sprintn
#define xprintf qse_wxprintf 

static const qse_wchar_t w_hex2ascii[] = QSE_WT("0123456789abcdefghijklmnopqrstuvwxyz");
#define hex2ascii(hex)  (w_hex2ascii[hex])

#include "printf.h"

int qse_wprintf (const char_t *fmt, ...)
{
	va_list ap;
	int n;
	va_start (ap, fmt);
	n = qse_wxprintf (fmt, put_wchar, put_mchar, QSE_NULL, ap);
	va_end (ap);
	return n;
}

int qse_wvprintf (const char_t* fmt, va_list ap)
{
	return qse_wxprintf (fmt, put_wchar, put_mchar, QSE_NULL, ap);
}
