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
