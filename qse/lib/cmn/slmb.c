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

#include <qse/cmn/slmb.h>
#include "mem.h"

#if !defined(QSE_HAVE_CONFIG_H)
#	if defined(_WIN32) || defined(__OS2__) || defined(__DOS__)
#		define HAVE_WCHAR_H
#		define HAVE_STDLIB_H
#		define HAVE_MBRLEN
#		define HAVE_MBRTOWC
#		define HAVE_WCRTOMB
#	elif defined(macintosh) && defined(__MWERKS__)
#		define HAVE_WCHAR_H
#		define HAVE_STDLIB_H
#	endif
#endif

#if defined(HAVE_WCHAR_H)
#	include <wchar.h>
#endif
#if defined(HAVE_STDLIB_H)
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
	/* not supported */
	return 0;
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
	/* not supported */
	return 0;
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
	/* not supported */
	return 0;
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
	/*qse_mbstate_t state = { { 0, } };*/
	qse_mbstate_t state;
	QSE_MEMSET (&state, 0, QSE_SIZEOF(state));
	return qse_slmbrtoslwc (mb, mbl, wc, &state);
}

qse_size_t qse_slwctoslmb (qse_wchar_t wc, qse_mchar_t* mb, qse_size_t mbl)
{
	/*qse_mbstate_t state = { { 0, } };*/
	qse_mbstate_t state;
	QSE_MEMSET (&state, 0, QSE_SIZEOF(state));
	return qse_slwcrtoslmb (wc, mb, mbl, &state);
}

qse_size_t qse_slmblen (const qse_mchar_t* mb, qse_size_t mbl)
{
	/*qse_mbstate_t state = { { 0, } };*/
	qse_mbstate_t state;
	QSE_MEMSET (&state, 0, QSE_SIZEOF(state));
	return qse_slmbrlen (mb, mbl, &state);
}

qse_size_t qse_slmblenmax (void)
{
#if defined(_WIN32)
	/* Windows doesn't handle utf8 properly even when your code page
	 * is CP_UTF8(65001). you should use functions in utf8.c for utf8 
	 * handling on windows. 2 is the maximum for DBCS encodings. */
	return 2; 

#elif defined(MB_CUR_MAX)

	return MB_CUR_MAX;

#elif (QSE_SIZEOF_WCHAR_T == QSE_SIZEOF_MCHAR_T)

	/* no proper multibyte string support */
	return 1;	

#else
	/* fallback max utf8 value */
	return 6;
#endif
}

