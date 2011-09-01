/*
 * $Id: str-end.c 556 2011-08-31 15:43:46Z hyunghwan.chung $
 *
    Copyright 2006-2011 Chung, Hyung-Hwan.
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

qse_mchar_t* qse_mbsend (const qse_mchar_t* str, const qse_mchar_t* sub)
{
	return qse_mbsxnend (str, qse_mbslen(str), sub, qse_mbslen(sub));
}

qse_mchar_t* qse_mbsxend (
	const qse_mchar_t* str, qse_size_t len, const qse_mchar_t* sub)
{
	return qse_mbsxnend (str, len, sub, qse_mbslen(sub));
}

qse_mchar_t* qse_mbsnend (
	const qse_mchar_t* str, const qse_mchar_t* sub, qse_size_t len)
{
	return qse_mbsxnend (str, qse_mbslen(str), sub, len);
}

qse_mchar_t* qse_mbsxnend (
	const qse_mchar_t* str, qse_size_t len1, 
	const qse_mchar_t* sub, qse_size_t len2)
{
	const qse_mchar_t* end1, * end2;

	if (len2 > len1) return QSE_NULL;

	end1 = str + len1;
	end2 = sub + len2;

	while (end2 > sub)
	{
		if (end1 <= str) return QSE_NULL;
		if (*(--end1) != *(--end2)) return QSE_NULL;
	}
	
	/* returns the pointer to the match start */
	return (qse_mchar_t*)end1;
}

qse_wchar_t* qse_wcsend (const qse_wchar_t* str, const qse_wchar_t* sub)
{
	return qse_wcsxnend (str, qse_wcslen(str), sub, qse_wcslen(sub));
}

qse_wchar_t* qse_wcsxend (
	const qse_wchar_t* str, qse_size_t len, const qse_wchar_t* sub)
{
	return qse_wcsxnend (str, len, sub, qse_wcslen(sub));
}

qse_wchar_t* qse_wcsnend (
	const qse_wchar_t* str, const qse_wchar_t* sub, qse_size_t len)
{
	return qse_wcsxnend (str, qse_wcslen(str), sub, len);
}

qse_wchar_t* qse_wcsxnend (
	const qse_wchar_t* str, qse_size_t len1, 
	const qse_wchar_t* sub, qse_size_t len2)
{
	const qse_wchar_t* end1, * end2;

	if (len2 > len1) return QSE_NULL;

	end1 = str + len1;
	end2 = sub + len2;

	while (end2 > sub)
	{
		if (end1 <= str) return QSE_NULL;
		if (*(--end1) != *(--end2)) return QSE_NULL;
	}
	
	/* returns the pointer to the match start */
	return (qse_wchar_t*)end1;
}
