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
#include <qse/cmn/chr.h>

#define IS_MDELIM(x,delim) (QSE_ISMSPACE(x) || (x) == delim)
#define IS_WDELIM(x,delim) (QSE_ISWSPACE(x) || (x) == delim)

const qse_mchar_t* qse_mbsword (const qse_mchar_t* str, const qse_mchar_t* word, qse_mchar_t extra_delim)
{
	/* find a full word in a string */

	const qse_mchar_t* ptr = str;

	if (extra_delim == QSE_MT('\0')) extra_delim = QSE_MT(' ');
	do
	{
		const qse_mchar_t* s;

		while (IS_MDELIM(*ptr,extra_delim)) ptr++;
		if (*ptr == QSE_MT('\0')) return QSE_NULL;

		s = ptr;
		while (*ptr != QSE_MT('\0') && !IS_MDELIM(*ptr,extra_delim)) ptr++;

		if (qse_mbsxcmp (s, ptr - s, word) == 0) return s;
	}
	while (*ptr != QSE_MT('\0'));

	return QSE_NULL;
}

const qse_mchar_t* qse_mbsxword (const qse_mchar_t* str, qse_size_t len, const qse_mchar_t* word, qse_mchar_t extra_delim)
{
	/* find a full word in a string */

	const qse_mchar_t* ptr = str;
	const qse_mchar_t* end = str + len;
	const qse_mchar_t* s;

	if (extra_delim == QSE_MT('\0')) extra_delim = QSE_MT(' ');
	do
	{
		while (ptr < end && IS_MDELIM(*ptr,extra_delim)) ptr++;
		if (ptr >= end) return QSE_NULL;

		s = ptr;
		while (ptr < end && !IS_MDELIM(*ptr,extra_delim)) ptr++;

		if (qse_mbsxcmp (s, ptr - s, word) == 0) return s;
	}
	while (ptr < end);

	return QSE_NULL;
}

const qse_mchar_t* qse_mbscaseword (const qse_mchar_t* str, const qse_mchar_t* word, qse_mchar_t extra_delim)
{
	/* find a full word in a string */

	const qse_mchar_t* ptr = str;

	if (extra_delim == QSE_MT('\0')) extra_delim = QSE_MT(' ');
	do
	{
		const qse_mchar_t* s;

		while (IS_MDELIM(*ptr,extra_delim)) ptr++;
		if (*ptr == QSE_MT('\0')) return QSE_NULL;

		s = ptr;
		while (*ptr != QSE_MT('\0') && !IS_MDELIM(*ptr,extra_delim)) ptr++;

		if (qse_mbsxcasecmp (s, ptr - s, word) == 0) return s;
	}
	while (*ptr != QSE_MT('\0'));

	return QSE_NULL;
}

const qse_mchar_t* qse_mbsxcaseword (const qse_mchar_t* str, qse_size_t len, const qse_mchar_t* word, qse_mchar_t extra_delim)
{
	const qse_mchar_t* ptr = str;
	const qse_mchar_t* end = str + len;
	const qse_mchar_t* s;

	if (extra_delim == QSE_MT('\0')) extra_delim = QSE_MT(' ');
	do
	{
		while (ptr < end && IS_MDELIM(*ptr,extra_delim)) ptr++;
		if (ptr >= end) return QSE_NULL;

		s = ptr;
		while (ptr < end && !IS_MDELIM(*ptr,extra_delim)) ptr++;

		if (qse_mbsxcasecmp (s, ptr - s, word) == 0) return s;
	}
	while (ptr < end);

	return QSE_NULL;
}

const qse_wchar_t* qse_wcsword (const qse_wchar_t* str, const qse_wchar_t* word, qse_wchar_t extra_delim)
{
	/* find a full word in a string */

	const qse_wchar_t* ptr = str;

	if (extra_delim == QSE_WT('\0')) extra_delim = QSE_WT(' ');
	do
	{
		const qse_wchar_t* s;

		while (IS_WDELIM(*ptr,extra_delim)) ptr++;
		if (*ptr == QSE_WT('\0')) return QSE_NULL;

		s = ptr;
		while (*ptr != QSE_WT('\0') && !IS_WDELIM(*ptr,extra_delim)) ptr++;

		if (qse_wcsxcmp (s, ptr - s, word) == 0) return s;
	}
	while (*ptr != QSE_WT('\0'));

	return QSE_NULL;
}

const qse_wchar_t* qse_wcsxword (const qse_wchar_t* str, qse_size_t len, const qse_wchar_t* word, qse_wchar_t extra_delim)
{
	/* find a full word in a string */

	const qse_wchar_t* ptr = str;
	const qse_wchar_t* end = str + len;
	const qse_wchar_t* s;

	if (extra_delim == QSE_WT('\0')) extra_delim = QSE_WT(' ');
	do
	{
		while (ptr < end && IS_WDELIM(*ptr,extra_delim)) ptr++;
		if (ptr >= end) return QSE_NULL;

		s = ptr;
		while (ptr < end && !IS_WDELIM(*ptr,extra_delim)) ptr++;

		if (qse_wcsxcmp (s, ptr - s, word) == 0) return s;
	}
	while (ptr < end);

	return QSE_NULL;
}

const qse_wchar_t* qse_wcscaseword (const qse_wchar_t* str, const qse_wchar_t* word, qse_wchar_t extra_delim)
{
	/* find a full word in a string */

	const qse_wchar_t* ptr = str;

	if (extra_delim == QSE_WT('\0')) extra_delim = QSE_WT(' ');
	do
	{
		const qse_wchar_t* s;

		while (IS_WDELIM(*ptr,extra_delim)) ptr++;
		if (*ptr == QSE_WT('\0')) return QSE_NULL;

		s = ptr;
		while (*ptr != QSE_WT('\0') && !IS_WDELIM(*ptr,extra_delim)) ptr++;

		if (qse_wcsxcasecmp (s, ptr - s, word) == 0) return s;
	}
	while (*ptr != QSE_WT('\0'));

	return QSE_NULL;
}

const qse_wchar_t* qse_wcsxcaseword (const qse_wchar_t* str, qse_size_t len, const qse_wchar_t* word, qse_wchar_t extra_delim)
{
	const qse_wchar_t* ptr = str;
	const qse_wchar_t* end = str + len;
	const qse_wchar_t* s;

	if (extra_delim == QSE_WT('\0')) extra_delim = QSE_WT(' ');
	do
	{
		while (ptr < end && IS_WDELIM(*ptr,extra_delim)) ptr++;
		if (ptr >= end) return QSE_NULL;

		s = ptr;
		while (ptr < end && !IS_WDELIM(*ptr,extra_delim)) ptr++;

		if (qse_wcsxcasecmp (s, ptr - s, word) == 0) return s;
	}
	while (ptr < end);

	return QSE_NULL;
}
