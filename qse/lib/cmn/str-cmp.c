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
		s1++; s2++;
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
