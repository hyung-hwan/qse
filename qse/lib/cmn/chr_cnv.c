/*
 * $Id: chr_cnv.c 396 2011-03-14 15:40:35Z hyunghwan.chung $
 *
    Copyright 2006-2009 Chung, Hyung-Hwan.
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
#include "mem.h"

#if !defined(QSE_HAVE_CONFIG_H)
#	if defined(_WIN32) || defined(__OS2__)
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

qse_size_t qse_mblen (const qse_mchar_t* mb, qse_size_t mblen)
{
#ifdef HAVE_MBRLEN
	size_t n;
	mbstate_t mbs = { 0 };

	n = mbrlen (mb, mblen, &mbs);
	if (n == 0) return 1; /* a null character */

	if (n == (size_t)-1) return 0; /* invalid sequence */
	if (n == (size_t)-2) return mblen + 1; /* incomplete sequence */

	return (qse_size_t)n;

	#if 0
	n = mblen (mb, mblen, &mbs);
	if (n == 0) return 1; /* a null character */
	if (n == (size_t)-1) return 0; /* invalid or incomplete sequence */
	return (qse_size_t)n;
	#endif
#else
	#error #### NOT SUPPORTED ####
#endif
}

qse_size_t qse_mbtowc (const qse_mchar_t* mb, qse_size_t mblen, qse_wchar_t* wc)
{
#ifdef HAVE_MBRTOWC
	size_t n;
	mbstate_t mbs = { 0 };

	n = mbrtowc (wc, mb, mblen, &mbs);
	if (n == 0) 
	{
		*wc = QSE_WT('\0');
		return 1;
	}

	if (n == (size_t)-1) return 0; /* invalid sequence */
	if (n == (size_t)-2) return mblen + 1; /* incomplete sequence */
	return (qse_size_t)n;
#else
	#error #### NOT SUPPORTED ####
#endif
}

qse_size_t qse_wctomb (qse_wchar_t wc, qse_mchar_t* mb, qse_size_t mblen)
{
#ifdef HAVE_WCRTOMB
	size_t n;
	mbstate_t mbs = { 0 };

/* man mbsinit
 * For 8-bit encodings, all states are equivalent to  the  initial  state.
 * For multibyte encodings like UTF-8, EUC-*, BIG5 or SJIS, the wide char‐
 * acter to  multibyte  conversion  functions  never  produce  non-initial
 * states,  but  the multibyte to wide-character conversion functions like
 * mbrtowc(3) do produce non-initial states when interrupted in the middle
 * of a character.
 */

#ifdef _SCO_DS
/* SCO defines MB_CUR_MAX as shown below:
 * extern unsigned char __ctype[];
 * #define MB_CUR_MAX	((int)__ctype[520])
 * Some hacks are needed for compilation with a C89 compiler. */
#	undef MB_CUR_MAX
#	define MB_CUR_MAX 32
#endif

	if (mblen < MB_CUR_MAX)
	{
		qse_mchar_t buf[MB_CUR_MAX];

		n = wcrtomb (buf, wc, &mbs);
		if (n > mblen) return mblen + 1; /* buffer to small */
		if (n == (size_t)-1) return 0; /* illegal character */

		QSE_MEMCPY (mb, buf, mblen);
	}
	else
	{
		n = wcrtomb (mb, wc, &mbs);
		if (n > mblen) return mblen + 1; /* buffer to small */
		if (n == (size_t)-1) return 0; /* illegal character */
	}

	return n;
#else
	#error #### NOT SUPPORTED ####
#endif
}

