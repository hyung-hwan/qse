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
#include "mem.h"


/*---------------------------------------------------------------
 * multi-byte string to number conversion 
 *---------------------------------------------------------------*/
int qse_mbstoi (const qse_mchar_t* mbs, int base)
{
	int v;
	QSE_MBSTONUM (v, mbs, QSE_NULL, base);
	return v;
}

long qse_mbstol (const qse_mchar_t* mbs, int base)
{
	long v;
	QSE_MBSTONUM (v, mbs, QSE_NULL, base);
	return v;
}

unsigned int qse_mbstoui (const qse_mchar_t* mbs, int base)
{
	unsigned int v;
	QSE_MBSTONUM (v, mbs, QSE_NULL, base);
	return v;
}

unsigned long qse_mbstoul (const qse_mchar_t* mbs, int base)
{
	unsigned long v;
	QSE_MBSTONUM (v, mbs, QSE_NULL, base);
	return v;
}

int qse_mbsxtoi (const qse_mchar_t* mbs, qse_size_t len, int base)
{
	int v;
	QSE_MBSXTONUM (v, mbs, len, QSE_NULL, base);
	return v;
}

long qse_mbsxtol (const qse_mchar_t* mbs, qse_size_t len, int base)
{
	long v;
	QSE_MBSXTONUM (v, mbs, len, QSE_NULL, base);
	return v;
}

unsigned int qse_mbsxtoui (const qse_mchar_t* mbs, qse_size_t len, int base)
{
	unsigned int v;
	QSE_MBSXTONUM (v, mbs, len, QSE_NULL, base);
	return v;
}

unsigned long qse_mbsxtoul (const qse_mchar_t* mbs, qse_size_t len, int base)
{
	unsigned long v;
	QSE_MBSXTONUM (v, mbs, len, QSE_NULL, base);
	return v;
}

qse_int_t qse_mbstoint (const qse_mchar_t* mbs, int base)
{
	qse_int_t v;
	QSE_MBSTONUM (v, mbs, QSE_NULL, base);
	return v;
}

qse_long_t qse_mbstolong (const qse_mchar_t* mbs, int base)
{
	qse_long_t v;
	QSE_MBSTONUM (v, mbs, QSE_NULL, base);
	return v;
}

qse_uint_t qse_mbstouint (const qse_mchar_t* mbs, int base)
{
	qse_uint_t v;
	QSE_MBSTONUM (v, mbs, QSE_NULL, base);
	return v;
}

qse_ulong_t qse_mbstoulong (const qse_mchar_t* mbs, int base)
{
	qse_ulong_t v;
	QSE_MBSTONUM (v, mbs, QSE_NULL, base);
	return v;
}

qse_int_t qse_mbsxtoint (const qse_mchar_t* mbs, qse_size_t len, int base)
{
	qse_int_t v;
	QSE_MBSXTONUM (v, mbs, len, QSE_NULL, base);
	return v;
}

qse_long_t qse_mbsxtolong (const qse_mchar_t* mbs, qse_size_t len, int base)
{
	qse_long_t v;
	QSE_MBSXTONUM (v, mbs, len, QSE_NULL, base);
	return v;
}

qse_uint_t qse_mbsxtouint (const qse_mchar_t* mbs, qse_size_t len, int base)
{
	qse_uint_t v;
	QSE_MBSXTONUM (v, mbs, len, QSE_NULL, base);
	return v;
}

qse_ulong_t qse_mbsxtoulong (const qse_mchar_t* mbs, qse_size_t len, int base)
{
	qse_ulong_t v;
	QSE_MBSXTONUM (v, mbs, len, QSE_NULL, base);
	return v;
}


/*---------------------------------------------------------------
 * wide string to number conversion 
 *---------------------------------------------------------------*/
int qse_wcstoi (const qse_wchar_t* wcs, int base)
{
	int v;
	QSE_WCSTONUM (v, wcs, QSE_NULL, base);
	return v;
}

long qse_wcstol (const qse_wchar_t* wcs, int base)
{
	long v;
	QSE_WCSTONUM (v, wcs, QSE_NULL, base);
	return v;
}

unsigned int qse_wcstoui (const qse_wchar_t* wcs, int base)
{
	unsigned int v;
	QSE_WCSTONUM (v, wcs, QSE_NULL, base);
	return v;
}

unsigned long qse_wcstoul (const qse_wchar_t* wcs, int base)
{
	unsigned long v;
	QSE_WCSTONUM (v, wcs, QSE_NULL, base);
	return v;
}

int qse_wcsxtoi (const qse_wchar_t* wcs, qse_size_t len, int base)
{
	int v;
	QSE_WCSXTONUM (v, wcs, len, QSE_NULL, base);
	return v;
}

long qse_wcsxtol (const qse_wchar_t* wcs, qse_size_t len, int base)
{
	long v;
	QSE_WCSXTONUM (v, wcs, len, QSE_NULL, base);
	return v;
}

unsigned int qse_wcsxtoui (const qse_wchar_t* wcs, qse_size_t len, int base)
{
	unsigned int v;
	QSE_WCSXTONUM (v, wcs, len, QSE_NULL, base);
	return v;
}

unsigned long qse_wcsxtoul (const qse_wchar_t* wcs, qse_size_t len, int base)
{
	unsigned long v;
	QSE_WCSXTONUM (v, wcs, len, QSE_NULL, base);
	return v;
}

qse_int_t qse_wcstoint (const qse_wchar_t* wcs, int base)
{
	qse_int_t v;
	QSE_WCSTONUM (v, wcs, QSE_NULL, base);
	return v;
}

qse_long_t qse_wcstolong (const qse_wchar_t* wcs, int base)
{
	qse_long_t v;
	QSE_WCSTONUM (v, wcs, QSE_NULL, base);
	return v;
}

qse_uint_t qse_wcstouint (const qse_wchar_t* wcs, int base)
{
	qse_uint_t v;
	QSE_WCSTONUM (v, wcs, QSE_NULL, base);
	return v;
}

qse_ulong_t qse_wcstoulong (const qse_wchar_t* wcs, int base)
{
	qse_ulong_t v;
	QSE_WCSTONUM (v, wcs, QSE_NULL, base);
	return v;
}

qse_int_t qse_wcsxtoint (const qse_wchar_t* wcs, qse_size_t len, int base)
{
	qse_int_t v;
	QSE_WCSXTONUM (v, wcs, len, QSE_NULL, base);
	return v;
}

qse_long_t qse_wcsxtolong (const qse_wchar_t* wcs, qse_size_t len, int base)
{
	qse_long_t v;
	QSE_WCSXTONUM (v, wcs, len, QSE_NULL, base);
	return v;
}

qse_uint_t qse_wcsxtouint (const qse_wchar_t* wcs, qse_size_t len, int base)
{
	qse_uint_t v;
	QSE_WCSXTONUM (v, wcs, len, QSE_NULL, base);
	return v;
}

qse_ulong_t qse_wcsxtoulong (const qse_wchar_t* wcs, qse_size_t len, int base)
{
	qse_ulong_t v;
	QSE_WCSXTONUM (v, wcs, len, QSE_NULL, base);
	return v;
}

/*---------------------------------------------------------------
 * case conversion
 *---------------------------------------------------------------*/
qse_size_t qse_mbslwr (qse_mchar_t* str)
{
	qse_mchar_t* p = str;
	for (p = str; *p != QSE_MT('\0'); p++) *p = QSE_TOMLOWER (*p);
	return p - str;
}

qse_size_t qse_mbsupr (qse_mchar_t* str)
{
	qse_mchar_t* p = str;
	for (p = str; *p != QSE_MT('\0'); p++) *p = QSE_TOMUPPER (*p);
	return p - str;
}

qse_size_t qse_wcslwr (qse_wchar_t* str)
{
	qse_wchar_t* p = str;
	for (p = str; *p != QSE_WT('\0'); p++) *p = QSE_TOWLOWER (*p);
	return p - str;
}

qse_size_t qse_wcsupr (qse_wchar_t* str)
{
	qse_wchar_t* p = str;
	for (p = str; *p != QSE_WT('\0'); p++) *p = QSE_TOWUPPER (*p);
	return p - str;
}



/*---------------------------------------------------------------
 * Hexadecimal string conversion
 *---------------------------------------------------------------*/

int qse_mbshextobin (const qse_mchar_t* hex, qse_size_t hexlen, qse_uint8_t* buf, qse_size_t buflen)
{
	const qse_mchar_t* end = hex + hexlen;
	qse_size_t bi = 0;

	while (hex < end && bi < buflen)
	{
		int v;

		if (*hex >= QSE_MT('0') && *hex <= QSE_MT('9')) v = *hex - QSE_MT('0');
		else if (*hex >= QSE_MT('a') && *hex <= QSE_MT('f')) v = *hex - QSE_MT('a') + 10;
		else if (*hex >= QSE_MT('A') && *hex <= QSE_MT('F')) v = *hex - QSE_MT('A') + 10;
		else return -1;

		buf[bi] = buf[bi] * 16 + v;

		hex++;
		if (hex >= end) return -1;

		if (*hex >= QSE_MT('0') && *hex <= QSE_MT('9')) v = *hex - QSE_MT('0');
		else if (*hex >= QSE_MT('a') && *hex <= QSE_MT('f')) v = *hex - QSE_MT('a') + 10;
		else if (*hex >= QSE_MT('A') && *hex <= QSE_MT('F')) v = *hex - QSE_MT('A') + 10;
		else return -1;

		buf[bi] = buf[bi] * 16 + v;

		hex++;
		bi++;
	}

	return 0;
}


int qse_wcshextobin (const qse_wchar_t* hex, qse_size_t hexlen, qse_uint8_t* buf, qse_size_t buflen)
{
	const qse_wchar_t* end = hex + hexlen;
	qse_size_t bi = 0;

	while (hex < end && bi < buflen)
	{
		int v;

		if (*hex >= QSE_WT('0') && *hex <= QSE_WT('9')) v = *hex - QSE_WT('0');
		else if (*hex >= QSE_WT('a') && *hex <= QSE_WT('f')) v = *hex - QSE_WT('a') + 10;
		else if (*hex >= QSE_WT('A') && *hex <= QSE_WT('F')) v = *hex - QSE_WT('A') + 10;
		else return -1;

		buf[bi] = buf[bi] * 16 + v;

		hex++;
		if (hex >= end) return -1;

		if (*hex >= QSE_WT('0') && *hex <= QSE_WT('9')) v = *hex - QSE_WT('0');
		else if (*hex >= QSE_WT('a') && *hex <= QSE_WT('f')) v = *hex - QSE_WT('a') + 10;
		else if (*hex >= QSE_WT('A') && *hex <= QSE_WT('F')) v = *hex - QSE_WT('A') + 10;
		else return -1;

		buf[bi] = buf[bi] * 16 + v;

		hex++;
		bi++;
	}

	return 0;
}
