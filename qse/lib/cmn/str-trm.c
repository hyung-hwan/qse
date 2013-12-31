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

qse_mchar_t* qse_mbstrmx (qse_mchar_t* str, int opt)
{
	qse_mchar_t* p = str;

	if (*p != QSE_MT('\0')) 
	{
		qse_mchar_t* s = QSE_NULL, * e = QSE_NULL;

		do
		{
			if (!QSE_ISMSPACE(*p))
			{
				if (s == QSE_NULL) s = p;
				e = p;
			}
			p++;
		}
		while (*p != QSE_MT('\0'));


		if (e)
		{
			if (opt & QSE_MBSTRMX_RIGHT) e[1] = QSE_MT('\0');
			if (opt & QSE_MBSTRMX_LEFT) str = s;
		}
		else
		{
			/* the entire string need to ne deleted */
			if ((opt & QSE_MBSTRMX_RIGHT) || 
			    (opt && QSE_MBSTRMX_LEFT)) str[0] = QSE_MT('\0');
		}
	}

	return str;
}

qse_mchar_t* qse_mbsxtrmx (qse_mchar_t* str, qse_size_t* len, int opt)
{
	qse_mchar_t* p = str, * end = str + *len;

	if (p < end)
	{
		qse_mchar_t* s = QSE_NULL, * e = QSE_NULL;

		do
		{
			if (!QSE_ISMSPACE(*p))
			{
				if (s == QSE_NULL) s = p;
				e = p;
			}
			p++;
		}
		while (p < end);

		if (e)
		{
			if (opt & QSE_MBSTRMX_RIGHT) 
			{
				*len -= end - e - 1;
			}
			if (opt & QSE_MBSTRMX_LEFT) 
			{
				*len -= s - str;
				str = s;
			}
		}
		else
		{
			/* the entire string need to ne deleted */
			if ((opt & QSE_MBSTRMX_RIGHT) || 
			    (opt && QSE_MBSTRMX_LEFT)) *len = 0;
		}
	}

	return str;
}


qse_wchar_t* qse_wcstrmx (qse_wchar_t* str, int opt)
{
	qse_wchar_t* p = str;

	if (*p != QSE_MT('\0')) 
	{
		qse_wchar_t* s = QSE_NULL, * e = QSE_NULL;

		do
		{
			if (!QSE_ISWSPACE(*p))
			{
				if (s == QSE_NULL) s = p;
				e = p;
			}
			p++;
		}
		while (*p != QSE_MT('\0'));


		if (e)
		{
			if (opt & QSE_WCSTRMX_RIGHT) e[1] = QSE_MT('\0');
			if (opt & QSE_WCSTRMX_LEFT) str = s;
		}
		else
		{
			/* the entire string need to ne deleted */
			if ((opt & QSE_WCSTRMX_RIGHT) || 
			    (opt && QSE_WCSTRMX_LEFT)) str[0] = QSE_MT('\0');
		}
	}

	return str;
}

qse_wchar_t* qse_wcsxtrmx (qse_wchar_t* str, qse_size_t* len, int opt)
{
	qse_wchar_t* p = str, * end = str + *len;

	if (p < end)
	{
		qse_wchar_t* s = QSE_NULL, * e = QSE_NULL;

		do
		{
			if (!QSE_ISWSPACE(*p))
			{
				if (s == QSE_NULL) s = p;
				e = p;
			}
			p++;
		}
		while (p < end);

		if (e)
		{
			if (opt & QSE_WCSTRMX_RIGHT) 
			{
				*len -= end - e - 1;
			}
			if (opt & QSE_WCSTRMX_LEFT) 
			{
				*len -= s - str;
				str = s;
			}
		}
		else
		{
			/* the entire string need to ne deleted */
			if ((opt & QSE_WCSTRMX_RIGHT) || 
			    (opt && QSE_WCSTRMX_LEFT)) *len = 0;
		}
	}

	return str;
}

/* -------------------------------------------------------------- */

qse_size_t qse_mbstrm (qse_mchar_t* str)
{
	qse_mchar_t* p = str;
	qse_mchar_t* s = QSE_NULL, * e = QSE_NULL;

	while (*p != QSE_MT('\0')) 
	{
		if (!QSE_ISMSPACE(*p)) 
		{
			if (s == QSE_NULL) s = p;
			e = p;
		}
		p++;
	}

	if (e)
	{
		e[1] = QSE_MT('\0');
		if (str != s)
			QSE_MEMCPY (str, s, (e - s + 2) * QSE_SIZEOF(*str));
		return e - s + 1;
	}

	str[0] = QSE_MT('\0');
	return 0;
}

qse_size_t qse_mbsxtrm (qse_mchar_t* str, qse_size_t len)
{
	qse_mchar_t* p = str, * end = str + len;
	qse_mchar_t* s = QSE_NULL, * e = QSE_NULL;

	while (p < end) 
	{
		if (!QSE_ISMSPACE(*p)) 
		{
			if (s == QSE_NULL) s = p;
			e = p;
		}
		p++;
	}

	if (e != QSE_NULL) 
	{
		/* do not insert a terminating null */
		/*e[1] = QSE_MT('\0');*/
		if (str != s)
			QSE_MEMCPY (str, s, (e - s + 2) * QSE_SIZEOF(*str));
		return e - s + 1;
	}

	/* do not insert a terminating null */
	/*str[0] = QSE_MT('\0');*/
	return 0;
}

qse_size_t qse_wcstrm (qse_wchar_t* str)
{
	qse_wchar_t* p = str;
	qse_wchar_t* s = QSE_NULL, * e = QSE_NULL;

	while (*p != QSE_MT('\0')) 
	{
		if (!QSE_ISWSPACE(*p)) 
		{
			if (s == QSE_NULL) s = p;
			e = p;
		}
		p++;
	}

	if (e)
	{
		e[1] = QSE_MT('\0');
		if (str != s)
			QSE_MEMCPY (str, s, (e - s + 2) * QSE_SIZEOF(*str));
		return e - s + 1;
	}

	str[0] = QSE_MT('\0');
	return 0;
}

qse_size_t qse_wcsxtrm (qse_wchar_t* str, qse_size_t len)
{
	qse_wchar_t* p = str, * end = str + len;
	qse_wchar_t* s = QSE_NULL, * e = QSE_NULL;

	while (p < end) 
	{
		if (!QSE_ISWSPACE(*p)) 
		{
			if (s == QSE_NULL) s = p;
			e = p;
		}
		p++;
	}

	if (e != QSE_NULL) 
	{
		/* do not insert a terminating null */
		/*e[1] = QSE_MT('\0');*/
		if (str != s)
			QSE_MEMCPY (str, s, (e - s + 2) * QSE_SIZEOF(*str));
		return e - s + 1;
	}

	/* do not insert a terminating null */
	/*str[0] = QSE_MT('\0');*/
	return 0;
}
