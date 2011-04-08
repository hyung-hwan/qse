/*
 * $Id: str_utl.c 427 2011-04-07 06:46:25Z hyunghwan.chung $
 *
    Copyright 2006-2009 Chung, Hyung-Hwan.
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

qse_char_t* qse_strtrmx (qse_char_t* str, int opt)
{
	qse_char_t* p = str;
	qse_char_t* s = QSE_NULL, * e = QSE_NULL;

	while (*p != QSE_T('\0'))
	{
		if (!QSE_ISSPACE(*p))
		{
			if (s == QSE_NULL) s = p;
			e = p;
		}
		p++;
	}

	if (opt & QSE_STRTRMX_RIGHT) e[1] = QSE_T('\0');
	if (opt & QSE_STRTRMX_LEFT) str = s;

	return str;
}

qse_size_t qse_strtrm (qse_char_t* str)
{
	qse_char_t* p = str;
	qse_char_t* s = QSE_NULL, * e = QSE_NULL;

	while (*p != QSE_T('\0')) 
	{
		if (!QSE_ISSPACE(*p)) 
		{
			if (s == QSE_NULL) s = p;
			e = p;
		}
		p++;
	}

	if (e != QSE_NULL) 
	{
		e[1] = QSE_T('\0');
		if (str != s)
			QSE_MEMCPY (str, s, (e - s + 2) * QSE_SIZEOF(qse_char_t));
		return e - s + 1;
	}

	str[0] = QSE_T('\0');
	return 0;
}

qse_size_t qse_strxtrm (qse_char_t* str, qse_size_t len)
{
	qse_char_t* p = str, * end = str + len;
	qse_char_t* s = QSE_NULL, * e = QSE_NULL;

	while (p < end) 
	{
		if (!QSE_ISSPACE(*p)) 
		{
			if (s == QSE_NULL) s = p;
			e = p;
		}
		p++;
	}

	if (e != QSE_NULL) 
	{
		/* do not insert a terminating null */
		/*e[1] = QSE_T('\0');*/
		if (str != s)
			QSE_MEMCPY (str, s, (e - s + 2) * QSE_SIZEOF(qse_char_t));
		return e - s + 1;
	}

	/* do not insert a terminating null */
	/*str[0] = QSE_T('\0');*/
	return 0;
}

qse_size_t qse_strpac (qse_char_t* str)
{
	qse_char_t* p = str, * q = str;

	while (QSE_ISSPACE(*p)) p++;
	while (*p != QSE_T('\0')) 
	{
		if (QSE_ISSPACE(*p)) 
		{
			*q++ = *p++;
			while (QSE_ISSPACE(*p)) p++;
		}
		else *q++ = *p++;
	}

	if (q > str && QSE_ISSPACE(q[-1])) q--;
	*q = QSE_T('\0');

	return q - str;
}

qse_size_t qse_strxpac (qse_char_t* str, qse_size_t len)
{
	qse_char_t* p = str, * q = str, * end = str + len;
	int followed_by_space = 0;
	int state = 0;

	while (p < end) 
	{
		if (state == 0) 
		{
			if (!QSE_ISSPACE(*p)) 
			{
				*q++ = *p;
				state = 1;
			}
		}
		else if (state == 1) 
		{
			if (QSE_ISSPACE(*p)) 
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
