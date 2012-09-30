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
#include <qse/cmn/chr.h>

int qse_mbscmp (const qse_mchar_t* s1, const qse_mchar_t* s2)
{
	while (*s1 == *s2) 
	{
		if (*s1 == QSE_MT('\0')) return 0;
		s1++, s2++;
	}

	return (*s1 > *s2)? 1: -1;
}

int qse_mbsxcmp (const qse_mchar_t* s1, qse_size_t ln, const qse_mchar_t* s2)
{
	const qse_mchar_t* end = s1 + ln;
	while (s1 < end && *s2 != QSE_MT('\0') && *s1 == *s2) s1++, s2++;
	if (s1 == end && *s2 == QSE_MT('\0')) return 0;
	if (*s1 == *s2) return (s1 < end)? 1: -1;
	return (*s1 > *s2)? 1: -1;
}

int qse_mbsxncmp (
	const qse_mchar_t* s1, qse_size_t ln1, 
	const qse_mchar_t* s2, qse_size_t ln2)
{
	qse_mchar_t c1, c2;
	const qse_mchar_t* end1 = s1 + ln1;
	const qse_mchar_t* end2 = s2 + ln2;

	while (s1 < end1)
	{
		c1 = *s1;
		if (s2 < end2) 
		{
			c2 = *s2;
			if (c1 > c2) return 1;
			if (c1 < c2) return -1;
		}
		else return 1;
		s1++; s2++;
	}

	return (s2 < end2)? -1: 0;
}

int qse_mbscasecmp (const qse_mchar_t* s1, const qse_mchar_t* s2)
{
	while (QSE_TOMUPPER(*s1) == QSE_TOMUPPER(*s2)) 
	{
		if (*s1 == QSE_C('\0')) return 0;
		s1++, s2++;
	}

	return (QSE_TOMUPPER(*s1) > QSE_TOMUPPER(*s2))? 1: -1;
}

int qse_mbsxcasecmp (const qse_mchar_t* s1, qse_size_t ln, const qse_mchar_t* s2)
{
	qse_mchar_t c1, c2;
	const qse_mchar_t* end = s1 + ln;

	c1 = QSE_TOMUPPER(*s1); c2 = QSE_TOMUPPER(*s2);
	while (s1 < end && c2 != QSE_MT('\0') && c1 == c2) 
	{
		s1++; s2++;
		c1 = QSE_TOMUPPER(*s1); c2 = QSE_TOMUPPER(*s2);
	}
	if (s1 == end && c2 == QSE_MT('\0')) return 0;
	if (c1 == c2) return (s1 < end)? 1: -1;
	return (c1 > c2)? 1: -1;
}

int qse_mbsxncasecmp (
	const qse_mchar_t* s1, qse_size_t ln1, 
	const qse_mchar_t* s2, qse_size_t ln2)
{
	qse_mchar_t c1, c2;
	const qse_mchar_t* end1 = s1 + ln1;
	const qse_mchar_t* end2 = s2 + ln2;

	while (s1 < end1)
	{
		c1 = QSE_TOMUPPER (*s1); 
		if (s2 < end2) 
		{
			c2 = QSE_TOMUPPER (*s2);
			if (c1 > c2) return 1;
			if (c1 < c2) return -1;
		}
		else return 1;
		s1++; s2++;
	}

	return (s2 < end2)? -1: 0;
}


int qse_mbszcmp (const qse_mchar_t* s1, const qse_mchar_t* s2, qse_size_t n)
{
	if (n == 0) return 0;

	while (*s1 == *s2)
	{
		if (*s1 == QSE_MT('\0') || n == 1) return 0;
		s1++, s2++, n--;
	}

	return (*s1 > *s2)? 1: -1;
}

int qse_mbszcasecmp (const qse_mchar_t* s1, const qse_mchar_t* s2, qse_size_t n)
{
	if (n == 0) return 0;

	while (QSE_TOMUPPER(*s1) == QSE_TOMUPPER(*s2)) 
	{
		if (*s1 == QSE_MT('\0') || n == 1) return 0;
		s1++, s2++, n--;
	}

	return (QSE_TOMUPPER(*s1) > QSE_TOMUPPER(*s2))? 1: -1;
}

/* ------------------------------------------------------------- */

int qse_wcscmp (const qse_wchar_t* s1, const qse_wchar_t* s2)
{
	while (*s1 == *s2) 
	{
		if (*s1 == QSE_WT('\0')) return 0;
		s1++, s2++;
	}

	return (*s1 > *s2)? 1: -1;
}

int qse_wcsxcmp (const qse_wchar_t* s1, qse_size_t ln, const qse_wchar_t* s2)
{
	const qse_wchar_t* end = s1 + ln;
	while (s1 < end && *s2 != QSE_WT('\0') && *s1 == *s2) s1++, s2++;
	if (s1 == end && *s2 == QSE_WT('\0')) return 0;
	if (*s1 == *s2) return (s1 < end)? 1: -1;
	return (*s1 > *s2)? 1: -1;
}

int qse_wcsxncmp (
	const qse_wchar_t* s1, qse_size_t ln1, 
	const qse_wchar_t* s2, qse_size_t ln2)
{
	qse_wchar_t c1, c2;
	const qse_wchar_t* end1 = s1 + ln1;
	const qse_wchar_t* end2 = s2 + ln2;

	while (s1 < end1)
	{
		c1 = *s1;
		if (s2 < end2) 
		{
			c2 = *s2;
			if (c1 > c2) return 1;
			if (c1 < c2) return -1;
		}
		else return 1;
		s1++; s2++;
	}

	return (s2 < end2)? -1: 0;
}

int qse_wcscasecmp (const qse_wchar_t* s1, const qse_wchar_t* s2)
{
	while (QSE_TOWUPPER(*s1) == QSE_TOWUPPER(*s2)) 
	{
		if (*s1 == QSE_C('\0')) return 0;
		s1++, s2++;
	}

	return (QSE_TOWUPPER(*s1) > QSE_TOWUPPER(*s2))? 1: -1;
}

int qse_wcsxcasecmp (const qse_wchar_t* s1, qse_size_t ln, const qse_wchar_t* s2)
{
	qse_wchar_t c1, c2;
	const qse_wchar_t* end = s1 + ln;

	c1 = QSE_TOWUPPER(*s1); c2 = QSE_TOWUPPER(*s2);
	while (s1 < end && c2 != QSE_WT('\0') && c1 == c2) 
	{
		s1++; s2++;
		c1 = QSE_TOWUPPER(*s1); c2 = QSE_TOWUPPER(*s2);
	}
	if (s1 == end && c2 == QSE_WT('\0')) return 0;
	if (c1 == c2) return (s1 < end)? 1: -1;
	return (c1 > c2)? 1: -1;
}

int qse_wcsxncasecmp (
	const qse_wchar_t* s1, qse_size_t ln1, 
	const qse_wchar_t* s2, qse_size_t ln2)
{
	qse_wchar_t c1, c2;
	const qse_wchar_t* end1 = s1 + ln1;
	const qse_wchar_t* end2 = s2 + ln2;

	while (s1 < end1)
	{
		c1 = QSE_TOWUPPER (*s1); 
		if (s2 < end2) 
		{
			c2 = QSE_TOWUPPER (*s2);
			if (c1 > c2) return 1;
			if (c1 < c2) return -1;
		}
		else return 1;
		s1++; s2++;
	}

	return (s2 < end2)? -1: 0;
}

int qse_wcszcmp (const qse_wchar_t* s1, const qse_wchar_t* s2, qse_size_t n)
{
	if (n == 0) return 0;

	while (*s1 == *s2)
	{
		if (*s1 == QSE_WT('\0') || n == 1) return 0;
		s1++, s2++, n--;
	}

	return (*s1 > *s2)? 1: -1;
}

int qse_wcszcasecmp (const qse_wchar_t* s1, const qse_wchar_t* s2, qse_size_t n)
{
	if (n == 0) return 0;

	while (QSE_TOWUPPER(*s1) == QSE_TOWUPPER(*s2)) 
	{
		if (*s1 == QSE_WT('\0') || n == 1) return 0;
		s1++, s2++, n--;
	}

	return (QSE_TOWUPPER(*s1) > QSE_TOWUPPER(*s2))? 1: -1;
}
