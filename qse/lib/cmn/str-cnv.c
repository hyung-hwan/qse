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

int qse_strtoi (const qse_char_t* str)
{
	int v;
	QSE_STRTONUM (v, str, QSE_NULL, 10);
	return v;
}

long qse_strtol (const qse_char_t* str)
{
	long v;
	QSE_STRTONUM (v, str, QSE_NULL, 10);
	return v;
}

unsigned int qse_strtoui (const qse_char_t* str)
{
	unsigned int v;
	QSE_STRTONUM (v, str, QSE_NULL, 10);
	return v;
}

unsigned long qse_strtoul (const qse_char_t* str)
{
	unsigned long v;
	QSE_STRTONUM (v, str, QSE_NULL, 10);
	return v;
}

int qse_strxtoi (const qse_char_t* str, qse_size_t len)
{
	int v;
	QSE_STRXTONUM (v, str, len, QSE_NULL, 10);
	return v;
}

long qse_strxtol (const qse_char_t* str, qse_size_t len)
{
	long v;
	QSE_STRXTONUM (v, str, len, QSE_NULL, 10);
	return v;
}

unsigned int qse_strxtoui (const qse_char_t* str, qse_size_t len)
{
	unsigned int v;
	QSE_STRXTONUM (v, str, len, QSE_NULL, 10);
	return v;
}

unsigned long qse_strxtoul (const qse_char_t* str, qse_size_t len)
{
	unsigned long v;
	QSE_STRXTONUM (v, str, len, QSE_NULL, 10);
	return v;
}

qse_int_t qse_strtoint (const qse_char_t* str)
{
	qse_int_t v;
	QSE_STRTONUM (v, str, QSE_NULL, 10);
	return v;
}

qse_long_t qse_strtolong (const qse_char_t* str)
{
	qse_long_t v;
	QSE_STRTONUM (v, str, QSE_NULL, 10);
	return v;
}

qse_uint_t qse_strtouint (const qse_char_t* str)
{
	qse_uint_t v;
	QSE_STRTONUM (v, str, QSE_NULL, 10);
	return v;
}

qse_ulong_t qse_strtoulong (const qse_char_t* str)
{
	qse_ulong_t v;
	QSE_STRTONUM (v, str, QSE_NULL, 10);
	return v;
}

qse_int_t qse_strxtoint (const qse_char_t* str, qse_size_t len)
{
	qse_int_t v;
	QSE_STRXTONUM (v, str, len, QSE_NULL, 10);
	return v;
}

qse_long_t qse_strxtolong (const qse_char_t* str, qse_size_t len)
{
	qse_long_t v;
	QSE_STRXTONUM (v, str, len, QSE_NULL, 10);
	return v;
}

qse_uint_t qse_strxtouint (const qse_char_t* str, qse_size_t len)
{
	qse_uint_t v;
	QSE_STRXTONUM (v, str, len, QSE_NULL, 10);
	return v;
}

qse_ulong_t qse_strxtoulong (const qse_char_t* str, qse_size_t len)
{
	qse_ulong_t v;
	QSE_STRXTONUM (v, str, len, QSE_NULL, 10);
	return v;
}

/* case conversion */

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
