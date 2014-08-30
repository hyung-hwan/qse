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
