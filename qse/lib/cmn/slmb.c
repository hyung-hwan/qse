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

#include <qse/cmn/slmb.h>
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
#if defined(_WIN32)
#	include <windows.h>
#endif

qse_size_t qse_slwcrtoslmb (
	qse_wchar_t wc, qse_mchar_t* mb,
	qse_size_t mbl, qse_mbstate_t* state)
{
#if defined(_WIN32)
	int n;

	/* CP_THREAD_ACP results in ERROR_INVALID_PARAMETER
	 * on an old windows os like win95 */
	n = WideCharToMultiByte (
		CP_ACP/*CP_THREAD_ACP*/, 0 /*WC_ERR_INVALID_CHARS*/, 
		&wc, 1, mb, mbl, NULL, NULL);
	if (n == 0)
	{
		DWORD e = GetLastError();
		if (e == ERROR_INSUFFICIENT_BUFFER) return mbl + 1;
		/*if (e == ERROR_NO_UNICODE_TRANSLATION) return 0;*/
		/* treat all other erros as invalid unicode character */
	}

	return (qse_size_t)n;

#elif defined(HAVE_WCRTOMB)
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

qse_size_t qse_slmbrtoslwc (
	const qse_mchar_t* mb, qse_size_t mbl, 
	qse_wchar_t* wc, qse_mbstate_t* state)
{
#if defined(_WIN32)
	qse_size_t dbcslen;
	int n;

	QSE_ASSERT (mb != QSE_NULL);
	QSE_ASSERT (mbl > 0);

	dbcslen = IsDBCSLeadByteEx(CP_ACP/*CP_THREAD_ACP*/, *mb)? 2: 1;
	if (mbl < dbcslen) return mbl + 1; /* incomplete sequence */

	n = MultiByteToWideChar (
		CP_ACP/*CP_THREAD_ACP*/, MB_ERR_INVALID_CHARS, mb, dbcslen, wc, 1);
	if (n == 0) 
	{
		/*DWORD e = GetLastError();*/
		/*if (e == ERROR_NO_UNICODE_TRANSLATION) return 0;*/
		/*if (e == ERROR_INSUFFICIENT_BUFFER) return mbl + 1;*/
		return 0;
	}

	return dbcslen;
#elif defined(__sun__) && defined(HAVE_MBRTOWC)
	/* 
	 * Read comments in qse_slmbrlen().
	 */

	size_t n;

	QSE_ASSERT (mb != QSE_NULL);
	QSE_ASSERT (mbl > 0);

	if (wc)
	{
		n = mbrtowc (wc, mb, mbl, (mbstate_t*)state);
		if (n == 0) 
		{
			*wc = QSE_WT('\0');
			return 1;
		}
	}
	else
	{
		qse_wchar_t dummy;
		n = mbrtowc (&dummy, mb, mbl, (mbstate_t*)state);
		if (n == 0) return 1;
	}

	if (n == (size_t)-1) return 0; /* invalid sequence */
	if (n == (size_t)-2) return mbl + 1; /* incomplete sequence */
	return (qse_size_t)n;

#elif defined(HAVE_MBRTOWC)
	size_t n;

	QSE_ASSERT (mb != QSE_NULL);
	QSE_ASSERT (mbl > 0);

	n = mbrtowc (wc, mb, mbl, (mbstate_t*)state);
	if (n == 0) 
	{
		if (wc) *wc = QSE_WT('\0');
		return 1;
	}

	if (n == (size_t)-1) return 0; /* invalid sequence */
	if (n == (size_t)-2) return mbl + 1; /* incomplete sequence */
	return (qse_size_t)n;
#else
	#error #### NOT SUPPORTED ####
#endif
}

qse_size_t qse_slmbrlen (
	const qse_mchar_t* mb, qse_size_t mbl, qse_mbstate_t* state)
{
#if defined(_WIN32)
	qse_size_t dbcslen;

	QSE_ASSERT (mb != QSE_NULL);
	QSE_ASSERT (mbl > 0);

	/* IsDBCSLeadByte() or IsDBCSLeadByteEx() doesn't validate
	 * the actual sequence. So it can't actually detect an invalid 
	 * sequence. Thus, qse_slmbrtowc() may return a different length
	 * for an invalid sequence form qse_slmbrlen(). */
	dbcslen = IsDBCSLeadByteEx(CP_ACP/*CP_THREAD_ACP*/, *mb)? 2: 1;
	if (mbl < dbcslen) return mbl + 1; /* incomplete sequence */
	return dbcslen;

#elif defined(__sun__) && defined(HAVE_MBRLEN)
	/* on solaris 8, 
	 *   for a valid utf8 sequence on the utf8-locale,
	 *     mbrlen() returned -1.
	 *     mbrtowc(NULL, mbs, mbl, state) also returned -1.
	 *     mblen() returned the right length.
	 *     mbrtowc(wc, mbs, mbl, state) returned the right length.
	 *   for a cp949 sequence on the cp949 locale,
	 *     mbrlen() returned the right length.
	 *     mbrtowc(NULL, mbs, mbl, state) returned the right length.
	 *     mblen() returned the right length.
	 *     mbrtowc(wc, mbs, mbl, state) returned the right length.
	 *
	 * The problem is buggy mbrlen() that can't handle utf8 sequence 
	 * properly. here is my quick and dirty workaround for solaris.
	 *
	 * Newer solaris 9 and 10 or later should be also affected since
	 * i don't check any version or something.
	 *
	 * There could be other platforms with the same issue.
	 */

	/* TODO:
	 * it seems that solaris is not the only platform with
	 * this kind of a bug. 
	 *
	 *    checking this in autoconf doesn't solve the problem.
	 *    the underlying system could have fixed the problem already.
	 *
	 *    checking this during library initialization makes sense.
	 *    qse_slmbinit() or qse_initlib() tests if mblen() and mbrlen()
	 *    returns consistant results and arranges properly method 
	 *    for this slmb routine.
	 */
	return qse_slmbrtoslwc (mb, mbl, QSE_NULL, state);

#elif defined(HAVE_MBRLEN)
	size_t n;

	QSE_ASSERT (mb != QSE_NULL);
	QSE_ASSERT (mbl > 0);

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

/* man mbsinit
 * For 8-bit encodings, all states are equivalent to  the  initial  state.
 * For multibyte encodings like UTF-8, EUC-*, BIG5 or SJIS, the wide char‐
 * acter to  multibyte  conversion  functions  never  produce  non-initial
 * states,  but  the multibyte to wide-character conversion functions like
 * mbrtowc(3) do produce non-initial states when interrupted in the middle
 * of a character.
 */

qse_size_t qse_slmbtoslwc (const qse_mchar_t* mb, qse_size_t mbl, qse_wchar_t* wc)
{
	qse_mbstate_t state = { { 0, } };
	return qse_slmbrtoslwc (mb, mbl, wc, &state);
}

qse_size_t qse_slwctoslmb (qse_wchar_t wc, qse_mchar_t* mb, qse_size_t mbl)
{
	qse_mbstate_t state = { { 0, } };
	return qse_slwcrtoslmb (wc, mb, mbl, &state);
}

qse_size_t qse_slmblen (const qse_mchar_t* mb, qse_size_t mbl)
{
	qse_mbstate_t state = { { 0, } };
	return qse_slmbrlen (mb, mbl, &state);
}

qse_size_t qse_slmblenmax (void)
{
#if defined(_WIN32)
	/* Windows doesn't handle utf8 properly even when your code page
	 * is CP_UTF8(65001). you should use functions in utf8.c for utf8 
	 * handleing on windows. 2 is the maximum for DBCS encodings. */
	return 2; 
#else
	return MB_CUR_MAX;
#endif
}
