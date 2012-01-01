/*
 * $Id: chr-cnv.c 556 2011-08-31 15:43:46Z hyunghwan.chung $
 *
    Copyright 2006-2011 Chung, Hyung-Hwan.
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

#include <qse/cmn/mbwc.h>
#include <qse/cmn/utf8.h>
#include "mem.h"

#if !defined(QSE_HAVE_CONFIG_H)
#	if defined(_WIN32) || defined(__OS2__) || defined(__DOS__)
#		define HAVE_WCHAR_H
#		define HAVE_STDLIB_H
#		define HAVE_MBRLEN
#		define HAVE_MBRTOWC
#		define HAVE_WCRTOMB
#	endif
#endif

#ifdef HAVE_WCHAR_H
#	include <wchar.h>
#endif
#ifdef HAVE_STDLIB_H
#	include <stdlib.h>
#endif

qse_size_t qse_mbrlen (
	const qse_mchar_t* mb, qse_size_t mbl, qse_mbstate_t* state)
{
#if defined(HAVE_MBRLEN)
	size_t n;

	n = mbrlen (mb, mbl, (mbstate_t*)state);
	if (n == 0) return 1; /* a null character */

	if (n == (size_t)-1) return 0; /* invalid sequence */
	if (n == (size_t)-2) return mbl + 1; /* incomplete sequence */

	return (qse_size_t)n;

	#if 0
	n = mblen (mb, mbl);
	if (n == (size_t)-1) return 0; /* invalid or incomplete sequence */
	if (n == 0) return 1; /* a null character */
	return (qse_size_t)n;
	#endif
#else
	#error #### NOT SUPPORTED ####
#endif
}

qse_size_t qse_mbrtowc (
	const qse_mchar_t* mb, qse_size_t mbl, 
	qse_wchar_t* wc, qse_mbstate_t* state)
{
#if defined(HAVE_MBRTOWC)
	size_t n;

	n = mbrtowc (wc, mb, mbl, (mbstate_t*)state);
	if (n == 0) 
	{
		*wc = QSE_WT('\0');
		return 1;
	}

	if (n == (size_t)-1) return 0; /* invalid sequence */
	if (n == (size_t)-2) return mbl + 1; /* incomplete sequence */
	return (qse_size_t)n;
#else
	#error #### NOT SUPPORTED ####
#endif
}

qse_size_t qse_wcrtomb (
	qse_wchar_t wc, qse_mchar_t* mb,
	qse_size_t mbl, qse_mbstate_t* state)
{
#if defined(HAVE_WCRTOMB)
	size_t n;

	if (mbl < QSE_MBLEN_MAX)
	{
		/* the buffer given is too small. try conversion on 
		 * a temporary buffer large enough to handle all locales 
		 * and copy the result to the original buffer.
		 */
		qse_mchar_t buf[QSE_MBLEN_MAX];

		n = wcrtomb (buf, wc, (mbstate_t*)state);
		/* it's important that n is checked againt (size_t)-1
		 * before againt mbl. n > mbl is true if n is (size_t)-1.
		 * if the check comes later, i won't have a chance to
		 * determine the case of an illegal character */
		if (n == (size_t)-1) return 0; /* illegal character */
		if (n > mbl) return mbl + 1; /* buffer to small */

		QSE_MEMCPY (mb, buf, mbl);
	}
	else
	{
		n = wcrtomb (mb, wc, (mbstate_t*)state);
		if (n == (size_t)-1) return 0; /* illegal character */
		if (n > mbl) return mbl + 1; /* buffer to small */
	}

	return n; /* number of bytes written to the buffer */
#else
	#error #### NOT SUPPORTED ####
#endif
}

/* man mbsinit
 * For 8-bit encodings, all states are equivalent to  the  initial  state.
 * For multibyte encodings like UTF-8, EUC-*, BIG5 or SJIS, the wide char‐
 * acter to  multibyte  conversion  functions  never  produce  non-initial
 * states,  but  the multibyte to wide-character conversion functions like
 * mbrtowc(3) do produce non-initial states when interrupted in the middle
 * of a character.
 */
qse_size_t qse_mblen (const qse_mchar_t* mb, qse_size_t mbl)
{
	qse_mbstate_t state = { { 0, } };
	return qse_mbrlen (mb, mbl, &state);
}

qse_size_t qse_mbtowc (const qse_mchar_t* mb, qse_size_t mbl, qse_wchar_t* wc)
{
	qse_mbstate_t state = { { 0, } };
	return qse_mbrtowc (mb, mbl, wc, &state);
}

qse_size_t qse_wctomb (qse_wchar_t wc, qse_mchar_t* mb, qse_size_t mbl)
{
	qse_mbstate_t state = { { 0, } };
	return qse_wcrtomb (wc, mb, mbl, &state);
}

int qse_mbcurmax (void)
{
/* TODO: consider other encodings */	
	return (QSE_UTF8LEN_MAX > MB_CUR_MAX)? QSE_UTF8LEN_MAX: MB_CUR_MAX;
}
