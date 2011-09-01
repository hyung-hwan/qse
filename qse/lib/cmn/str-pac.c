/*
 * $Id: str-pac.c 556 2011-08-31 15:43:46Z hyunghwan.chung $
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
#include <qse/cmn/chr.h>

qse_size_t qse_mbspac (qse_mchar_t* str)
{
	qse_mchar_t* p = str, * q = str;

	while (QSE_ISMSPACE(*p)) p++;
	while (*p != QSE_MT('\0')) 
	{
		if (QSE_ISMSPACE(*p)) 
		{
			*q++ = *p++;
			while (QSE_ISMSPACE(*p)) p++;
		}
		else *q++ = *p++;
	}

	if (q > str && QSE_ISMSPACE(q[-1])) q--;
	*q = QSE_MT('\0');

	return q - str;
}

qse_size_t qse_mbsxpac (qse_mchar_t* str, qse_size_t len)
{
	qse_mchar_t* p = str, * q = str, * end = str + len;
	int followed_by_space = 0;
	int state = 0;

	while (p < end) 
	{
		if (state == 0) 
		{
			if (!QSE_ISMSPACE(*p)) 
			{
				*q++ = *p;
				state = 1;
			}
		}
		else if (state == 1) 
		{
			if (QSE_ISMSPACE(*p)) 
			{
				if (!followed_by_space) 
				{
					followed_by_space = 1;
					*q++ = *p;
				}
			}
			else 
			{
				followed_by_space = 0;
				*q++ = *p;	
			}
		}

		p++;
	}

	return (followed_by_space) ? (q - str -1): (q - str);
}

qse_size_t qse_wcspac (qse_wchar_t* str)
{
	qse_wchar_t* p = str, * q = str;

	while (QSE_ISWSPACE(*p)) p++;
	while (*p != QSE_WT('\0')) 
	{
		if (QSE_ISWSPACE(*p)) 
		{
			*q++ = *p++;
			while (QSE_ISWSPACE(*p)) p++;
		}
		else *q++ = *p++;
	}

	if (q > str && QSE_ISWSPACE(q[-1])) q--;
	*q = QSE_WT('\0');

	return q - str;
}

qse_size_t qse_wcsxpac (qse_wchar_t* str, qse_size_t len)
{
	qse_wchar_t* p = str, * q = str, * end = str + len;
	int followed_by_space = 0;
	int state = 0;

	while (p < end) 
	{
		if (state == 0) 
		{
			if (!QSE_ISWSPACE(*p)) 
			{
				*q++ = *p;
				state = 1;
			}
		}
		else if (state == 1) 
		{
			if (QSE_ISWSPACE(*p)) 
			{
				if (!followed_by_space) 
				{
					followed_by_space = 1;
					*q++ = *p;
				}
			}
			else 
			{
				followed_by_space = 0;
				*q++ = *p;	
			}
		}

		p++;
	}

	return (followed_by_space) ? (q - str -1): (q - str);
}
